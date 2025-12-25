#pragma once
#define NOMINMAX
#include <containers/detail/containers_internal.h>
#include <algorithm>
#include <utility.h>
#include <optional>
#include <xmemory>
#include <span>

namespace voxory {
  namespace containers {
    /**
    * @brief Single-threaded circular buffer with dynamic capacity.
    *
    *        Provides efficient push/pop and assignment operations with support for
    *        wrap-around storage and allocator-aware memory management.
    *
    *        The container is optimized for single-threaded use and makes no attempt
    *        to provide internal synchronization.
    *
    * @note Not thread-safe. All operations must be externally synchronized
    *       if used across threads. Concurrent access without synchronization
    *       results in undefined behavior.
    *
    * @tparam T     Element type.
    * @tparam Alloc Allocator used for allocation and element construction/destruction,
    *               with std::construct_at / std::destroy_at as fallback.
    */
    template<typename T, typename Alloc = std::allocator<T>>
    class ring_buffer {
      using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
      using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
    public:
      using allocator_type = Alloc;
      using allocator_traits = std::allocator_traits<Alloc>;
      using value_type = T;
      using size_type = typename allocator_traits::size_type;
      using pointer = typename allocator_traits::pointer;
      using const_pointer = typename allocator_traits::const_pointer;
      using reference = T&;
      using const_reference = const T&;
      using difference = std::ptrdiff_t;
      using cleanup_guard = internal::cleanup_guard<ring_buffer>;
      friend cleanup_guard;

      // --- ctors & dtor ---
      ring_buffer(const allocator_type& alloc = allocator_type()) : _pair(FirstOneSecondArgs{}, alloc), _allow_overwrite(true), _capacity(0) {};

      /**
       * @brief Constructor for the ring_buffer class.
       * @note Reserves memory for the buffer but does not construct any elements.
       * @param capacity The number of elements the buffer can hold.
       * @param alloc Allocator for memory management (defaults to the standard allocator).
       */
      ring_buffer(size_type capacity, const allocator_type& alloc = allocator_type()) : _pair(FirstOneSecondArgs{}, alloc), _allow_overwrite(true), _capacity(0) {
        _allocate_buffer(capacity);
      };

      ring_buffer(const ring_buffer&) = delete;
      /**
       * @brief Constructs a ring_buffer from other obj. Strong guarantee.
       * @param o Other object to move in.
       */
      ring_buffer(ring_buffer&& o) noexcept : 
        _pair(FirstOneSecondArgs{}, std::move(o.get_allocator()), std::move(o._pair._second)), _allow_overwrite(o._allow_overwrite), _capacity(o._capacity) {

      };
      /**
       * @brief Dtor for the ring_buffer class.
       */
      ~ring_buffer() {
        _cleanup();
      };

      ring_buffer& operator=(const ring_buffer&) = delete;
      /**
       * @brief Constructs a ring_buffer from other obj. Strong guarantee.
       * @param o Other object to move in.
       */
      ring_buffer& operator=(ring_buffer&& o) noexcept {
        if (this == &o) return *this;
        auto& this_alloc = get_allocator();
        auto& o_alloc = o.get_allocator();

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
          _cleanup();
          this_alloc = std::move(o_alloc);
          _pair._second = std::move(o._pair._second);
          return *this;
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            _cleanup();
            _pair._second = std::move(o._pair._second);
            return *this;
          }
          else {
            if (this_alloc == o_alloc)
            {
              _cleanup();
              _pair._second = std::move(o._pair._second);
              return *this;
            }
            else
            {
              _assign_move(o);
              o._cleanup();
              return *this;
            }
          }
        }
      };

      // --- capacity/info ---
      /*
       * @brief Capacity getter.
       * @return capacity of ring buffer.
       */
      FORCE_INLINE CONSTEXPR size_type capacity() const noexcept {
        return _capacity;
      };
      /*
       * @brief Returns the size of the ring buffer.
       * @note Count of constructed elements may be greater than this value.
       * @return The number of currently active elements in the ring buffer.
       */
      CONSTEXPR size_type size() const noexcept {
        return _pair._second._size;
      };
      /**
       * @brief Checks if the ring buffer is empty.
       * @return True if the ring buffer has no elements; otherwise, false.
       */
      CONSTEXPR bool empty() const noexcept {
        return size() == 0;
      };
      /**
       * @brief Checks if the ring buffer is full.
       * @return True if the ring buffer has reached its capacity; otherwise, false.
       */
      CONSTEXPR bool full() const noexcept {
        return size() == _capacity;
      };

      // --- data ---

      /**
       * @brief Retrieves a pointer to the raw data of the ring buffer.
       * @return A pointer to the start of data stored in the ring buffer.
       */
      NODISCARD CONSTEXPR pointer data() noexcept  {
        return _pair._second._data;
      };
      NODISCARD CONSTEXPR const_pointer data() const noexcept  {
        return _pair._second._data;
      };

      /**
       * @brief Gets a reference to the allocator used by the ring buffer.
       * @return A reference to the allocator for memory management.
       */
      NODISCARD CONSTEXPR allocator_type& get_allocator() noexcept {
        return _pair.first();
      };

      /**
       * @brief Gets a constant reference to the allocator used by the ring buffer.
       * @return A constant reference to the allocator for memory management.
       */
      NODISCARD CONSTEXPR const allocator_type& get_allocator() const noexcept {
        return _pair.first();
      };

      // --- overwrite policy ---
      /**
       * @brief Sets the overwrite behavior of the ring buffer.
       * @param allow A boolean indicating whether overwriting of elements is allowed.
       */
      CONSTEXPR FORCE_INLINE void set_overwrite(bool allow) noexcept {
        _allow_overwrite = allow;
      };

      /**
       * @brief Checks if overwriting is allowed in the ring buffer.
       * @return True if overwriting of elements is permitted; otherwise, false.
       */
      CONSTEXPR FORCE_INLINE bool overwrite_allowed() const noexcept {
        return _allow_overwrite;
      };

      // --- single element operations (works with move-only types) ---
      // If the buffer is full and overwrite is set to false, it will 
      // dynamically increase the capacity; otherwise, it will 
      // overwrite the old elements, which may result in data loss.

      /**
       * @brief Adds a copy of an element to the ring buffer.
       * @brief If the buffer is full, it may increase capacity or overwrite the oldest element,
       *        potentially resulting in data loss.
       * @param v The element to be added.
       */
      CONSTEXPR void push_back(const T& v) {
        _emplace_back(v);
      };

      /**
       * @brief Adds a movable element to the ring buffer.
       * @brief If the buffer is full, it may increase capacity or overwrite the oldest element,
       *        potentially resulting in data loss.
       * @param v The element to be added (moved).
       */
      CONSTEXPR void push_back(T&& v) {
        _emplace_back(std::move(v));
      };
      /**
       * @brief Non-throwing pop operation.
       * @note basic guarantee
       * @return An optional<T> containing the removed element if successful; otherwise, an empty optional.
       */
      CONSTEXPR std::optional<T> try_pop_front() {
        auto& alloc = _pair.first();
        auto& data = _pair._second;
        auto& first = data._first;

        if (data._size == 0) return std::nullopt;

        pointer to_read = first;
        T local(std::move(*to_read));
        internal::destroy_at(to_read, alloc);
        first = _append(first, 1);
        --data._size;

        return std::optional<T>(std::move(local));
      }

      /**
       * @brief Peeks at the front element without removing it.
       * @note The pointer remains valid until a pop operation occurs.
       * @return A pointer to the front element in the buffer, or nullptr if empty.
       */
      CONSTEXPR T* peek_front() noexcept {
        auto& data = _pair._second;
        auto& first = data._first;

        if (data._size == 0) return nullptr;
        return first;
      };

      /**
       * @brief Peeks at the front element without removing it.
       * @note The pointer remains valid until a pop operation occurs.
       * @return A pointer to the front element in the buffer, or nullptr if empty.
       */
      CONSTEXPR const T* peek_front() const noexcept {
        auto& data = _pair._second;
        auto& first = data._first;

        if (data._size == 0) return nullptr;
        return first;
      };

      /**
       * @brief Index access operator for the ring buffer.
       * @note This operator allows read-write access to elements logically indexed from 0 to size() - 1.
       * @param idx The logical index (0 is the oldest element).
       * @return A reference to the element at the specified index.
       */
      NODISCARD CONSTEXPR reference operator[](size_type idx) noexcept {
        return *_append(_pair._second._first, idx);
      };

      /**
       * @brief Index access operator for the ring buffer.
       * @note This operator allows read-only access to elements logically indexed from 0 to size() - 1.
       * @param idx The logical index (0 is the oldest element).
       * @return A const reference to the element at the specified index.
       *
       */
      NODISCARD CONSTEXPR const_reference operator[](size_type idx) const noexcept {
        return *_append(_pair._second._first, idx);
      };

      /**
       * @brief Destroy all elements in the ring buffer, leaving it empty; does not deallocate buffer
       */
      CONSTEXPR void clear() noexcept(std::is_nothrow_destructible_v<value_type>) {
        auto& data = _pair._second;
        auto& first = data._first;
        auto& last = data._last;
        _destroy_range(first, last);
        first = nullptr;
        last = nullptr;
        data._size = 0;
      };
      
      /**
       * @brief Ensures that the ring buffer has at least the specified capacity.
       *
       * @note This operation changes the internal index layout (`first` / `last`) and
       * invalidates all pointers, references, and iterators to the elements.
       * 
       * @details
       * If the requested capacity is greater than the current one, the buffer
       * reallocates its storage. During reallocation, all existing elements are
       * moved into the new storage starting from the beginning of the buffer.
       * The relative positions of elements in the old storage are not preserved,
       * but their logical order (iteration order) remains unchanged.
       * 
       * @param new_capacity The minimum number of elements the buffer should be
       * able to hold without further reallocation.
       */
      CONSTEXPR void reserve(size_type new_capacity) {
        if (new_capacity > _capacity) {
          _reallocate(new_capacity);
        }
      }

      /**
       * @brief Reduces the capacity of the ring buffer to fit its current size.
       *
       * @details
       * Reallocates the internal storage so that the capacity becomes exactly
       * equal to the current number of elements in the buffer.
       *
       * All existing elements are preserved and moved into the new storage
       * contiguously starting from the beginning. The relative physical positions
       * of elements are not preserved, but their logical order remains unchanged.
       *
       * @note This operation changes the internal index layout (`first` / `last`) and
       * invalidates all pointers, references, and iterators to the elements.
       * If the buffer is empty, all storage may be released.
       *
       * Provides the strong exception guarantee.
       */
      CONSTEXPR void shrink_to_fit() {
        size_type sz = size();
        if (sz < _capacity) {
          _reallocate(sz);
        }
      }

      // --- bulk / zero-copy style API ---
      /**
       * @brief Provides writable memory spans for adding elements.
       * @return A pair of spans representing writable memory.
       */
      CONSTEXPR std::pair<std::span<T>, std::span<T>> write_spans_for_push_back() noexcept;

      /**
       * @brief Confirms that a specified number of elements were written into the spans.
       * @param n The number of elements that have been constructed or written.
       */
      CONSTEXPR void commit_write(size_type n) noexcept {
        _pair._second.last_ = _append(_pair._second.last_, n);
      }

      /**
       * @brief Provides readable memory spans for removing elements.
       * @return A pair of spans containing readable elements (const).
       */
      CONSTEXPR std::pair<std::span<const T>, std::span<const T>> read_spans_for_pop_front() const noexcept;

      /**
       * @brief Removes a specified number of elements from the buffer.
       * @note Destroys n elements, which may impact performance for complex objects.
       * @param n The number of elements to remove (destroy).
       */
      CONSTEXPR void commit_read(size_type n) noexcept {
        _destroy_n(_pair._second._first, n);
        _pair._second._first = _append(_pair._second._first, n);
      }

      // --- helpers ---

      /**
       * @brief Swaps the contents of two ring buffers.
       * @param other Another ring_buffer to swap with.
       */
      CONSTEXPR void swap(ring_buffer& other) noexcept {
        
      };

    private:

      CONSTEXPR FORCE_INLINE pointer _append(pointer current_ptr, size_type n) const noexcept {
        size_type offset = static_cast<size_type>(current_ptr - _pair._second._data);
        size_type new_offset = (offset + n) % _capacity;
        return _pair._second._data + new_offset;
      }

      //basic guarantee
      template<typename U>
      CONSTEXPR void _emplace_back_with_unused_capacity(U&& val) noexcept(std::is_nothrow_constructible_v<value_type, U>) {
        auto& alloc = get_allocator();
        auto& data = _pair._second;
        auto& last = data._last;
        auto& start = data._data;
        pointer end = data._data + _capacity;
        
        if (last + 1 <= end) {
          last = internal::construct_at(last, alloc, std::forward<U>(val));
        }
        else {
          last = internal::construct_at(start, alloc, std::forward<U>(val));
        }
        data._size++;
      }
      //basic guarantee
      template<typename U>
      CONSTEXPR void _emplace_back_overwrite(U&& val) noexcept(std::is_nothrow_assignable_v<value_type&, U>) {
        if (_pair._second._size != _capacity) return;
        auto& data = _pair._second;
        pointer first = data._first;
        pointer start = data._data;
        pointer end = start + _capacity;

        *first = std::forward<U>(val);

        pointer next = first + 1;
        data._last = next;
        data._first = (next == end) ? start : next;
      }

      constexpr inline size_type _calculate_grow(size_type capacity) noexcept {
        if (capacity == 0) {
          return 4;
        }
        else if (capacity < 8) {
          return capacity * 2; 
        }
        else if (capacity < 64) {
          return capacity + (capacity / 2); 
        }
        else {
          return capacity + (capacity / 4);
        }
      }

      template<typename U>
      CONSTEXPR void _emplace_back(U&& val) {
        auto& alloc = get_allocator();
        auto& data = _pair._second;
        auto& first = data._first;
        auto& last = data._last;
        auto& start = data._data;
        pointer end = data._data + _capacity;

        if(data._size < _capacity) {
          _emplace_back_with_unused_capacity(std::forward<U>(val));
        }
        else {
          if (_allow_overwrite == true) {
            _emplace_back_overwrite(std::forward<U>(val));
          }
          else {
            _reallocate(_calculate_grow(_capacity));
            _emplace_back_with_unused_capacity(std::forward<U>(val));
          }
        }
      };

      CONSTEXPR void _cleanup() {
        auto& this_data = _pair._second;
        auto& first = this_data._first;
        auto& last = this_data._last;

        if (first != nullptr)
        {
          _destroy_range(first, last);
          _deallocate_buffer();
        }
      };

      // if size > new_capacity = UB; strong guarantee
      CONSTEXPR void _reallocate(size_type new_capacity) {
        auto& this_alloc = get_allocator();
        auto& this_data = _pair._second;
        pointer new_data = this_alloc.allocate(new_capacity);
        pointer new_last = new_data;
        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          if (this_data._size != 0) new_last = _fill_uninitialized_from_ring(new_data, *this, internal::move_tag{});
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          if (this_data._size != 0) new_last = _fill_uninitialized_from_ring(new_data, *this, internal::copy_tag{});
        }
        else {
          if (this_data._size != 0) {
            internal::realloc_guard<allocator_type> guard(this_alloc, new_data, new_capacity);
            new_last = _fill_uninitialized_from_ring(new_data, *this, internal::copy_tag{});
            guard.release();
          }
        }
        this_data._data = new_data;
        this_data._first = new_data;
        this_data._last = new_last;
      }

      // if o_data._size == 0 then UB, must be verified at higher levels 
      template<typename Tag>
      CONSTEXPR pointer _fill_uninitialized_from_ring(pointer dest, ring_buffer& o, Tag&&)
        noexcept((std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) ? 
                  internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> : 
                  internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) 
      {
        auto& this_data = _pair._second;
        auto alloc = get_allocator();

        auto& o_data = o._pair._second;
        pointer o_first = o_data._first;
        pointer o_last = o_data._last;
        pointer o_start = o_data._data;
        pointer o_end = o_start + o._capacity;
        
        // place elements at the start of array; 
        // better for perfomance, elements are counted relative to the old element
        // TODO: UB
        if (o_first < o_last) {
          if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) {
            return internal::uninitialized_move(o_first, o_last, dest, alloc);
          }
          else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::copy_tag>) {
            return internal::uninitialized_copy(o_first, o_last, dest, alloc);
          }
        }
        else {
          if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) {
            dest = internal::uninitialized_move(o_first, o_end, dest, alloc);
            return internal::uninitialized_move(o_start, o_last, dest, alloc);
          }
          else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::copy_tag>) {
            dest = internal::uninitialized_copy(o_first, o_end, dest, alloc);
            return internal::uninitialized_copy(o_start, o_last, dest, alloc);
          }
        }
      }

      template<typename Tag>
      CONSTEXPR std::tuple<pointer, pointer, size_type> _allocate_fill_temp(ring_buffer& o, Tag&& tag) {
        auto& this_alloc = get_allocator();
        size_type o_capacity = o._capacity;
        pointer new_start = this_alloc.allocate(o_capacity);

        if (o._pair._second._size == 0) return std::make_tuple(new_start, new_start, 0 );
        // guard ensures new storage is released on exception (strong exception guarantee)
        internal::realloc_guard<allocator_type> guard(this_alloc, new_start, o_capacity);

        // copy/move elements into the new buffer; place them contiguously for better locality
        pointer new_last = _fill_uninitialized_from_ring(new_start, o, tag);

        guard.release();
        return std::make_tuple(new_start, new_last, o._pair._second._size);
      }
      // TODO: May cause undefined behavior for write/read spans. 
      // Consider creating internal classes Writer and Reader to manage span operations 
      // and ensure safe access to elements in the ring buffer.
      // strong guarantee
      CONSTEXPR void _cleanup_and_move_source(ring_buffer& o) 
        noexcept(internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> || 
                 internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
        auto& this_data = _pair._second;
        auto& this_alloc = get_allocator();

        auto& o_data = o._pair._second;
        pointer o_first = o_data._first;
        pointer o_last = o_data._last;
        pointer o_start = o_data._data;
        pointer o_end = o_start + o._capacity;
        size_type o_capacity = o._capacity;

        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          // move is noexcept, we don't need realloc_guard; just destroy old data and allocate new
          _cleanup(); // destroy old data, this_data._first/this_data._last = nullptr
          _allocate_buffer(o_capacity); // changes this_data._first 
          this_data._last = _fill_uninitialized_from_ring(this_data._first, o, internal::move_tag{});
          this_data._size = this_data._last - this_data._first;
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          // copy is noexcept, we don't need realloc_guard; just destroy old data and allocate new
          _cleanup(); // destroy old data, this_data._first/this_data._last = nullptr
          _allocate_buffer(o_capacity); // changes this_data._first
          this_data._last = _fill_uninitialized_from_ring(this_data._first, o, internal::copy_tag{});
          this_data._size = this_data._last - this_data._first;
        }
        else {
          // copy may throw, so we must allocate first and keep old data intact if it thrown
          auto [new_start, new_last, new_size] = _allocate_fill_temp(o, internal::copy_tag{});
          _cleanup();
          this_data._data = new_start;
          this_data._first = new_start;
          this_data._last = new_last;
          this_data._size = new_size;
          _capacity = o_capacity;
        }
      }

      CONSTEXPR void _destroy_and_move_source(ring_buffer& o)
        noexcept(internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> ||
                 internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
        auto& this_data = _pair._second;
        auto& this_alloc = get_allocator();

        auto& o_data = o._pair._second;
        pointer o_first = o_data._first;
        pointer o_last = o_data._last;
        pointer o_start = o_data._data;
        pointer o_end = o_start + o._capacity;
        size_type o_capacity = o._capacity;
        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          // move is noexcept, we don't need realloc_guard; just destroy old data
          _destroy_range(this_data._first, this_data._last);
          this_data._first = this_data._data;
          this_data._last = this_data._data;
          this_data._size = 0;
          if (o_data._size == 0) return;
          this_data._last = _fill_uninitialized_from_ring(this_data._first, o, internal::move_tag{});
          this_data._size = this_data._last - this_data._first;
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          // copy is noexcept, we don't need realloc_guard; just destroy old data
          _destroy_range(this_data._first, this_data._last);
          this_data._first = this_data._data;
          this_data._last = this_data._data;
          this_data._size = 0;
          if (o_data._size == 0) return;
          this_data._last = _fill_uninitialized_from_ring(this_data._first, o, internal::copy_tag{});
          this_data._size = this_data._last - this_data._first;
        }
        else {
          // copy may throw, so we must allocate first and keep old data intact if it thrown
          auto [new_start, new_last, new_size] = _allocate_fill_temp(o, internal::copy_tag{});
          _cleanup();
          this_data._data = new_start;
          this_data._first = new_start;
          this_data._last = new_last;
          this_data._size = new_size;
          _capacity = o_capacity;
        }
      }
      
      template<typename Tag>
      CONSTEXPR pointer _assign_helper(pointer src_first, pointer src_last, pointer dest)
        noexcept((std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) ?
          internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> :
          internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>)
      {
        auto& alloc = get_allocator();

        if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) {
          return internal::move_assign_n(src_first, src_last - src_first, dest, alloc);
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::copy_tag>) {
          return internal::copy_assign_n(src_first, src_last - src_first, dest, alloc);
        }
      }

      template<typename Tag>
      // does not verify capacity of objects, should be done on higher levels
      CONSTEXPR void _assign_move_helper(ring_buffer& o, Tag&& /*t*/) {
        auto& this_data = _pair._second;
        auto& this_alloc = get_allocator();
        size_type this_size = this_data._size;

        auto& o_data = o._pair._second;
        pointer o_first = o_data._first;
        pointer o_last = o_data._last;
        pointer o_start = o_data._data;
        pointer o_end = o_start + o._capacity;
        size_type o_size = o_data._size;

        pointer dst = this_data._first;
        pointer dst_start = this_data._data;
        pointer dst_end = dst_start + _capacity;
        pointer old_dst_last = this_data._last;

        pointer src_seg1_first = nullptr, src_seg1_last = nullptr;
        pointer src_seg2_first = nullptr, src_seg2_last = nullptr;
        bool has_seg2 = false;

        if (o_first < o_last) {
          src_seg1_first = o_first;
          src_seg1_last = o_last;
        } else if (o_size == 0) {
          _destroy_range(this_data._first, this_data._last);
          this_data._last = this_data._first;
          this_data._size = 0;
          return;
        }
        else {
          src_seg1_first = o_first;
          src_seg1_last = o_end;
          src_seg2_first = o_start;
          src_seg2_last = o_last;
          has_seg2 = true;
        }

        size_type assign_count = std::min(this_size, o_size);
        size_type construct_count = (o_size > this_size) ? (o_size - this_size) : 0;
        size_type remaining_assign = assign_count;

        auto advance_dst = [&dst, dst_start, dst_end](size_type n) noexcept -> void {
          dst += n;
          if (dst >= dst_end) dst = dst_start + (dst - dst_end);
        };

        auto process_assign_from_segment = [&](pointer seg_first, pointer seg_last) {
          pointer s = seg_first;
          while (s < seg_last && remaining_assign > 0) {
            size_type src_avail = static_cast<size_type>(seg_last - s);
            size_type dst_avail = static_cast<size_type>(dst_end - dst);
            size_type take = std::min({ src_avail, dst_avail, remaining_assign });

            pointer new_dst = _assign_helper<Tag>(s, s + take, dst);
            s += take;
            if (static_cast<size_type>(new_dst - dst) == take) {
              dst = new_dst;
              if (dst == dst_end) dst = dst_start;
            }
            else {
              advance_dst(take);
            }

            remaining_assign -= take;
          }
          return s;
        };
        // noexcept assign_move/assign_copy => strong exception guarantee,
        // otherwise the guarantee is only basic.
        if (remaining_assign > 0) {
          pointer pos = process_assign_from_segment(src_seg1_first, src_seg1_last);
          if (remaining_assign > 0 && has_seg2) {
            process_assign_from_segment(src_seg2_first, src_seg2_last);
          }
        }

        pointer new_last = dst;

        if (this_size > o_size) {
          _destroy_range(new_last, old_dst_last);
          this_data._last = new_last;
          this_data._size = o_size;
          return;
        }

        // basic guarantee
        if (construct_count > 0) {
          size_type skipped = assign_count;
          pointer s1 = src_seg1_first;
          pointer s2 = has_seg2 ? src_seg2_first : nullptr;

          size_type s1_len = static_cast<size_type>(src_seg1_last - src_seg1_first);
          pointer cur_src;

          if (skipped < s1_len) {
            cur_src = s1 + skipped;
          }
          else {
            skipped -= s1_len;
            if (has_seg2) {
              cur_src = s2 + skipped;
            }
            else {
              // shouldn't happen, fallback:
              cur_src = src_seg1_last;
            }
          }

          size_type need = construct_count;

          while (need > 0) {
            pointer cur_seg_end;
            if (cur_src >= src_seg1_first && cur_src < src_seg1_last) {
              cur_seg_end = src_seg1_last;
            }
            else {
              cur_seg_end = src_seg2_last;
            }

            size_type src_avail = static_cast<size_type>(cur_seg_end - cur_src);
            size_type dst_avail = static_cast<size_type>(dst_end - dst);
            size_type take = std::min(src_avail, dst_avail);
            take = std::min<size_type>(take, need);
            //TODO: don't like it
            if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) {
              dst = internal::uninitialized_move_n(this_alloc, dst, take, cur_src);
            }
            else {
              dst = internal::uninitialized_copy_n(this_alloc, dst, take, cur_src);
            }

            // advance src
            cur_src += take;
            if (cur_src == src_seg1_last && has_seg2) {
              cur_src = src_seg2_first;
            }
            if (dst == dst_end) dst = dst_start;

            need -= take;
          }

          new_last = dst;
          this_data._last = new_last;
          this_data._size = o_size;
          return;
        }

        _destroy_range(new_last, old_dst_last);
        this_data._last = new_last;
        this_data._size = o_size;
        return;
      }


      // Provides strong or basic exception guarantee depending on element move/copy properties; 
      // helper itself gives basic guarantee.
      CONSTEXPR void _assign_move(ring_buffer& o) {
        size_type this_capacity = _capacity;
        size_type o_capacity = o._capacity;
        size_type this_size = size();
        size_type o_size = o.size();

        //strong guarantee
        if (this_size == 0 || o_size == 0) {
          if (o_size != 0) {
            _cleanup_and_move_source(o);
          }
          else {
            if (o_capacity != _capacity)
            {
              _deallocate_buffer();
              _allocate_buffer(o_capacity);
            }
          }
          return;
        }
        //strong guarantee
        if (o_capacity > _capacity)
        {
          _cleanup_and_move_source(o);
          return;
        }
        //basic guarantee
        if constexpr (internal::is_nothrow_move_assignable_n_v<allocator_type, pointer>) {
          _assign_move_helper(o, internal::move_tag{});
        }
        else {
          //basic guarantee
          if constexpr (internal::is_nothrow_copy_assignable_n_v<allocator_type, pointer>) {
            _assign_move_helper(o, internal::copy_tag{});
          }
          //strong guarantee
          else {
            //destroy elements in current buffer and uninitialized_move from source with strong guarantee
            _destroy_and_move_source(o);
            return;
          }
        }
      }

      CONSTEXPR void _allocate_buffer(size_type count) {
        auto& data = _pair._second;
        auto& alloc = get_allocator();

        data._data = alloc.allocate(count);

        data._first = data._data;
        data._last = data._data;
        data._size = 0;
        _capacity = count;
      }

      CONSTEXPR void _deallocate_buffer() {
        auto& alloc = get_allocator();
        auto& data = _pair._second;
        auto& start = data._data;
        auto& first = data._first;
        auto& last = data._last;

        alloc.deallocate(start, _capacity);
        start = nullptr;
        first = nullptr;
        last = nullptr;
        data._size = 0;
        _capacity = 0;
      }

      CONSTEXPR void _destroy_range(pointer first, pointer last) noexcept(std::is_nothrow_destructible_v<value_type>) {
        if constexpr (std::is_trivially_destructible_v<value_type>) {
          return;
        }
        else {
          if (_pair._second._size == 0) return;
          auto& alloc = _pair.first();
          auto& data = _pair._second;
          pointer start = data._data;
          pointer end = start + _capacity;

          auto destroy_seq = [&](pointer b, pointer e) noexcept(std::is_nothrow_destructible_v<value_type>) {
            if constexpr (type_traits::has_destroy_v<allocator_type, pointer>) {
              for (; b != e; ++b) alloc.destroy(b);
            }
            else {
              for (; b != e; ++b) std::destroy_at(b);
            }
          };

          if (last > first) {
            destroy_seq(first, last);
          }
          else {
            destroy_seq(first, end);
            destroy_seq(start, last);
          }
        }
      }

      CONSTEXPR void _destroy_n(pointer first, size_type n) noexcept(std::is_nothrow_destructible_v<value_type>) {
        if constexpr (std::is_trivially_destructible_v<value_type>) {
          return;
        }
        else {
          if (n == 0) return;
          auto& alloc = _pair.first();
          auto& data = _pair._second;
          pointer start = data._data;
          pointer end = start + _capacity;

          auto destroy_seq = [&](pointer b, pointer e) noexcept(std::is_nothrow_destructible_v<value_type>) {
            if constexpr (type_traits::has_destroy_v<allocator_type, pointer>) {
              for (; b != e; ++b) alloc.destroy(b);
            }
            else {
              for (; b != e; ++b) std::destroy_at(b);
            }
          };

          if (first+n <= end)
          {
            destroy_seq(first, first + n);
          }
          else {
            size_type to_destroy = n - (end - first);
            destroy_seq(first, end);
            destroy_seq(start, start+to_destroy);
          }
        }
      }

      struct _storage {
        _storage() noexcept : _data(nullptr), _last(nullptr), _first(nullptr), _size(0) {};
        _storage(_storage&& o) noexcept : _data(std::exchange(o._data, nullptr)), _first(std::exchange(o._first, nullptr)), _last(std::exchange(o._last, nullptr)), _size(std::exchange(o._size, 0)) {};
        _storage(const _storage& o) noexcept : _data(nullptr), _first(nullptr), _last(nullptr), _size(o._size) {};
        _storage& operator=(_storage&& o) noexcept {
          if (this == std::addressof(o)) return *this;
          _data = std::exchange(o._data, nullptr);
          _last = std::exchange(o._last, nullptr);
          _first = std::exchange(o._first, nullptr);
          _size = std::exchange(o._size, 0);
          return *this;
        };

        CONSTEXPR void swap(_storage& o) noexcept {
          _data = std::exchange(o._data, _data);
          _last = std::exchange(o._last, _last);
          _first = std::exchange(o._first, _first);
          _size = std::exchange(o._size, _size);
        };

        // start of array
        pointer _data;
        // first constructed element
        pointer _first;
        // last constructed element
        pointer _last;
        // size of constructed elements
        size_type _size;
      };

      utility::compressed_pair<allocator_type, _storage> _pair;
      size_type _capacity;
      bool _allow_overwrite;
    };

    /// SPSC-optimized variant (lightweight, lock-free for 1P/1C)
    template<typename T, typename Alloc = std::allocator<T>>
    class spsc_ring_buffer {
    public:
      using allocator_type = Alloc;
      using allocator_traits = std::allocator_traits<Alloc>;
      using value_type = T;
      using size_type = typename allocator_traits::size_type;
      using pointer = typename allocator_traits::pointer;
      using reference = T&;

      explicit spsc_ring_buffer(size_type _capacitypower_of_two, Alloc = {});
      bool push(const T&);   // returns false if full (no overwrite)
      bool push(T&&);
      bool pop(T& out);      // returns false if empty
      // transaction API for zero-copy
      std::optional<std::pair<std::span<T>, std::span<T>>> start_write(size_type max_elems);
      void commit_write(size_type n);
      std::optional<std::pair<std::span<const T>, std::span<const T>>> start_read(size_type max_elems);
      void commit_read(size_type n);
      // diagnostics
      size_type capacity() const noexcept;
      size_type size() const noexcept;
      bool empty() const noexcept;
      bool full() const noexcept;
    };
  } // namespace containers
}