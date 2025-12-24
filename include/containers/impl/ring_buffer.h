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
      using reference = T&;
      using const_reference = const T&;
      using difference = std::ptrdiff_t;
      using cleanup_guard = internal::cleanup_guard<ring_buffer>;
      friend cleanup_guard;

      // --- ctors & dtor ---
      ring_buffer(const allocator_type& alloc = allocator_type()) : pair_(FirstOneSecondArgs{}, alloc), allow_overwrite_(true), capacity_(0) {};

      /**
       * @brief Constructor for the ring_buffer class.
       * @note Reserves memory for the buffer but does not construct any elements.
       * @param capacity The number of elements the buffer can hold.
       * @param alloc Allocator for memory management (defaults to the standard allocator).
       */
      ring_buffer(size_type capacity, const allocator_type& alloc = allocator_type()) : pair_(FirstOneSecondArgs{}, alloc), allow_overwrite_(true), capacity_(0) {
        allocate_buffer(capacity);
      };

      ring_buffer(const ring_buffer&) = delete;
      /**
       * @brief Constructs a ring_buffer from other obj. Strong guarantee.
       * @param o Other object to move in.
       */
      ring_buffer(ring_buffer&& o) noexcept : pair_(FirstOneSecondArgs{}, o.get_allocator(), std::move(o.pair_.second_)), allow_overwrite_(o.allow_overwrite_), capacity_(o.capacity_) {

      };
      /**
       * @brief Dtor for the ring_buffer class.
       */
      ~ring_buffer() = default;

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
          cleanup();
          this_alloc = std::move(o_alloc);
          pair_.second_ = std::move(o.pair_.second_);
          return *this;
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            cleanup();
            pair_.second_ = std::move(o.pair_.second_);
            return *this;
          }
          else {
            if (al == o_al)
            {
              cleanup();
              pair_.second_ = std::move(o.pair_.second_);
              return *this;
            }
            else
            {
              assign_move(o);
              o.cleanup();
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
      FORCE_INLINE CONSTEXPR size_type capacity() {
        return capacity_;
      };
      /*
       * @brief Returns the size of the ring buffer.
       * @note Count of constructed elements may be greater than this value.
       * @return The number of currently active elements in the ring buffer.
       */
      CONSTEXPR size_type size() const noexcept {
        if (pair_.second_.head_ >= pair_.second_.tail_) return pair_.second_.head_ - pair_.second_.tail_;
        return capacity_ - (pair_.second_.tail_ - pair_.second_.head_);
      };
      /**
       * @brief Checks if the ring buffer is empty.
       * @return True if the ring buffer has no elements; otherwise, false.
       */
      CONSTEXPR bool empty() const noexcept {
        return pair_.second_.head_ == pair_.second_.tail_;
      };
      /**
       * @brief Checks if the ring buffer is full.
       * @return True if the ring buffer has reached its capacity; otherwise, false.
       */
      CONSTEXPR bool full() const noexcept {
        return size() == capacity_;
      };

      // --- data ---

      /**
       * @brief Retrieves a pointer to the raw data of the ring buffer.
       * @return A pointer to the start of data stored in the ring buffer.
       */
      NODISCARD CONSTEXPR pointer data() {
        return pair_.second_.data_;
      };

      /**
       * @brief Gets a reference to the allocator used by the ring buffer.
       * @return A reference to the allocator for memory management.
       */
      NODISCARD CONSTEXPR allocator_type& get_allocator() noexcept {
        return pair_.first();
      };

      /**
       * @brief Gets a constant reference to the allocator used by the ring buffer.
       * @return A constant reference to the allocator for memory management.
       */
      NODISCARD CONSTEXPR const allocator_type& get_allocator() const noexcept {
        return pair_.first();
      };

      // --- overwrite policy ---
      /**
       * @brief Sets the overwrite behavior of the ring buffer.
       * @param allow A boolean indicating whether overwriting of elements is allowed.
       */
      CONSTEXPR FORCE_INLINE void set_overwrite(bool allow) noexcept {
        allow_overwrite_ = allow;
      };

      /**
       * @brief Checks if overwriting is allowed in the ring buffer.
       * @return True if overwriting of elements is permitted; otherwise, false.
       */
      CONSTEXPR FORCE_INLINE bool overwrite_allowed() const noexcept {
        return allow_overwrite_;
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
        emplace_back(v);
      };

      /**
       * @brief Adds a movable element to the ring buffer.
       * @brief If the buffer is full, it may increase capacity or overwrite the oldest element,
       *        potentially resulting in data loss.
       * @param v The element to be added (moved).
       */
      CONSTEXPR void push_back(T&& v) {
        emplace_back(std::move(v));
      };
      /**
       * @brief Non-throwing pop operation.
       * @return An optional<T> containing the removed element if successful; otherwise, an empty optional.
       */
      CONSTEXPR std::optional<T> try_pop_front() noexcept;

      /**
       * @brief Peeks at the front element without removing it.
       * @note The pointer remains valid until a pop operation occurs.
       * @return A pointer to the front element in the buffer, or nullptr if empty.
       */
      CONSTEXPR T* peek_front() noexcept;

      /**
       * @brief Peeks at the front element without removing it.
       * @note The pointer remains valid until a pop operation occurs.
       * @return A pointer to the front element in the buffer, or nullptr if empty.
       */
      CONSTEXPR const T* peek_front() const noexcept;

      /**
       * @brief Index access operator for the ring buffer.
       * @note This operator allows read-write access to elements logically indexed from 0 to size() - 1.
       * @param idx The logical index (0 is the oldest element).
       * @return A reference to the element at the specified index.
       */
      NODISCARD CONSTEXPR reference operator[](size_type idx) noexcept {
        return pair_.second_.data_[append(pair_.second_.tail_, idx)];
      };

      /**
       * @brief Index access operator for the ring buffer.
       * @note This operator allows read-only access to elements logically indexed from 0 to size() - 1.
       * @param idx The logical index (0 is the oldest element).
       * @return A const reference to the element at the specified index.
       *
       */
      NODISCARD CONSTEXPR const_reference operator[](size_type idx) const noexcept {
        return pair_.second_.data_[append(pair_.second_.tail_, idx)];
      };

      /**
       * @brief Removes all elements from the ring buffer, leaving it empty.
       * @brief UB to use container afterwards.
       */
      CONSTEXPR void clear() {};

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
        pair_.second_.head_ = append(pair_.second_.head_, n);
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
        destroy_n(pair_.second_.first_, n);
        pair_.second_.tail_ = append(pair_.second_.tail_, n);
      }

      // --- helpers ---

      /**
       * @brief Swaps the contents of two ring buffers.
       * @param other Another ring_buffer to swap with.
       */
      CONSTEXPR void swap(ring_buffer& other) noexcept;

    private:

      CONSTEXPR FORCE_INLINE size_type append(size_type current_num, size_type n) const noexcept {
        return (current_num + n) % capacity_;
      }

      template<typename... Args>
      CONSTEXPR emplace_back_with_unused_capacity(Args&&... args) {
        auto& data = pair_.second_;
        auto& last = data.last_;
        auto& start = data.data_;
        pointer end = data.data_ + capacity_;
        size_type tail = this_data.tail_;

      }

      template<typename... Args>
      CONSTEXPR void emplace_back(Args&&... args) {
        
      };

      CONSTEXPR void cleanup() {
        auto& this_data = pair_.second_;
        auto& first = this_data.first_;
        auto& last = this_data.last_;

        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
          this_data.tail_ = 0;
          this_data.head_ = 0;
        }
      };

      // strong guarantee
      CONSTEXPR void reallocate(size_type new_capacity) {
        auto& this_alloc = get_allocator();
        auto& this_data = pair_.second_;
        pointer new_data = this_alloc.allocate(new_capacity);
        pointer new_last = nullptr;
        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          new_last = fill_uninitialized_from_ring(new_data, *this, internal::move_tag);
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          new_last = fill_uninitialized_from_ring(new_data, *this, internal::copy_tag);
        }
        else {
          internal::realloc_guard<allocator_type> guard(this_alloc, new_data, new_capacity);
          new_last = fill_uninitialized_from_ring(new_data, *this, internal::copy_tag);
          guard.release();
        }
        this_data.data_ = new_data;
        this_data.first_ = new_data;
        this_data.last_ = new_last;
        this_data.tail_ = 0;
        this_data.head_ = new_last-new_data;
      }

      template<typename Tag>
      CONSTEXPR pointer fill_uninitialized_from_ring(pointer dest, ring_buffer& o, Tag&&) 
        noexcept((std::is_same_v<std::remove_cvref_t<Tag>, internal::move_tag>) ? 
                  internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> : 
                  internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) 
      {
        auto& this_data = pair_.second_;
        auto alloc = get_allocator();

        auto& o_data = o.pair_.second_;
        pointer o_first = o_data.first_;
        pointer o_last = o_data.last_;
        pointer o_start = o_data.data_;
        pointer o_end = o_start + o.capacity_;

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
      CONSTEXPR std::pair<pointer, pointer> allocate_fill_temp(ring_buffer& o, Tag&& tag) {
        auto& this_alloc = get_allocator();
        size_type o_capacity = o.capacity_;
        pointer new_start = this_alloc.allocate(o_capacity);
        pointer new_last = new_start;

        // guard ensures new storage is released on exception (strong exception guarantee)
        internal::realloc_guard<allocator_type> guard(this_alloc, new_start, o_capacity);

        // copy/move elements into the new buffer; place them contiguously for better locality
        new_last = fill_uninitialized_from_ring(new_start, o, tag);

        guard.release();
        return { new_start, new_last };
      }
      // TODO: May cause undefined behavior for write/read spans. 
      // Consider creating internal classes Writer and Reader to manage span operations 
      // and ensure safe access to elements in the ring buffer.
      // strong guarantee
      CONSTEXPR void cleanup_and_move_source(ring_buffer& o) 
        noexcept(internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> || 
                 internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
        auto& this_data = pair_.second_;
        auto& this_alloc = get_allocator();

        auto& o_data = o.pair_.second_;
        pointer o_first = o_data.first_;
        pointer o_last = o_data.last_;
        pointer o_start = o_data.data_;
        pointer o_end = o_start + o.capacity_;
        size_type o_capacity = o.capacity_;
        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          // move is noexcept, we don't need realloc_guard; just destroy old data and allocate new
          cleanup(); // destroy old data, this_data.first_/this_data.last_ = nullptr
          allocate_buffer(o_capacity); // changes this_data.first_ 
          this_data.last_ = fill_uninitialized_from_ring(this_data.first_, o, internal::move_tag{});
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          // copy is noexcept, we don't need realloc_guard; just destroy old data and allocate new
          cleanup(); // destroy old data, this_data.first_/this_data.last_ = nullptr
          allocate_buffer(o_capacity); // changes this_data.first_
          this_data.last_ = fill_uninitialized_from_ring(this_data.first_, o, internal::copy_tag{});
        }
        else {
          // copy may throw, so we must allocate first and keep old data intact if it thrown
          auto [new_start, new_last] = allocate_fill_temp(o, internal::copy_tag{});
          cleanup();
          this_data.data_ = new_start;
          this_data.last_ = new_last;
          capacity_ = o_capacity;
        }
        this_data.head_ = 0;
        this_data.tail_ = this_data.last_ - this_data.data_ - 1;
      }

      CONSTEXPR void destroy_and_move_source(ring_buffer& o)
        noexcept(internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer> ||
                 internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
        auto& this_data = pair_.second_;
        auto& this_alloc = get_allocator();

        auto& o_data = o.pair_.second_;
        pointer o_first = o_data.first_;
        pointer o_last = o_data.last_;
        pointer o_start = o_data.data_;
        pointer o_end = o_start + o.capacity_;
        size_type o_capacity = o.capacity_;
        if constexpr (internal::is_nothrow_uninitialized_moveable_v<allocator_type, pointer>)
        {
          // move is noexcept, we don't need realloc_guard; just destroy old data
          destroy_range(this_data.first_, this_data.last_);
          this_data.first_ = this_data.data_;
          this_data.last_ = this_data.data_;
          this_data.last_ = fill_uninitialized_from_ring(this_data.first_, o, internal::move_tag{});
        }
        else if constexpr (internal::is_nothrow_uninitialized_copyable_v<allocator_type, pointer>) {
          // copy is noexcept, we don't need realloc_guard; just destroy old data
          destroy_range(this_data.first_, this_data.last_);
          this_data.first_ = this_data.data_;
          this_data.last_ = this_data.data_;
          this_data.last_ = fill_uninitialized_from_ring(this_data.first_, o, internal::copy_tag{});
        }
        else {
          // copy may throw, so we must allocate first and keep old data intact if it thrown
          auto [new_start, new_last] = allocate_fill_temp(o, internal::copy_tag{});
          cleanup();
          this_data.data_ = new_start;
          this_data.last_ = new_last;
          capacity_ = o_capacity;
        }
        this_data.head_ = 0;
        this_data.tail_ = this_data.last_ - this_data.data_ - 1;
      }
      
      template<typename Tag>
      CONSTEXPR pointer assign_helper(pointer src_first, pointer src_last, pointer dest)
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
      CONSTEXPR void assign_move_helper(ring_buffer& o, Tag&& /*t*/) {
        auto& this_data = pair_.second_;
        auto& this_alloc = get_allocator();
        size_type this_size = size();

        auto& o_data = o.pair_.second_;
        pointer o_first = o_data.first_;
        pointer o_last = o_data.last_;
        pointer o_start = o_data.data_;
        pointer o_end = o_start + o.capacity_;
        size_type o_size = o.size();

        pointer dst = this_data.first_;
        pointer dst_start = this_data.start;
        pointer dst_end = dst_start + capacity_;
        pointer old_dst_last = this_data.last_;

        pointer src_seg1_first = nullptr, src_seg1_last = nullptr;
        pointer src_seg2_first = nullptr, src_seg2_last = nullptr;
        bool has_seg2 = false;

        if (o_first < o_last) {
          src_seg1_first = o_first;
          src_seg1_last = o_last;
        }
        else if (o_first == o_last) {
          destroy_range(this_data.first_, this_data.last_);
          this_data.last_ = this_data.first_;
          this_data.head_ = this_data.last_ - this_data.data_;
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

            pointer new_dst = assign_helper<Tag>(s, s + take, dst);
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
          destroy_range(new_last, old_dst_last);
          this_data.last_ = new_last;
          this_data.head_ = this_data.last_ - this_data.data_;
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
          this_data.last_ = new_last;
          this_data.head_ = this_data.last_ - this_data.data_;
          return;
        }

        destroy_range(new_last, old_dst_last);
        this_data.last_ = new_last;
        this_data.head_ = this_data.last_ - this_data.data_;
        return;
      }


      // Provides strong or basic exception guarantee depending on element move/copy properties; 
      // helper itself gives basic guarantee.
      CONSTEXPR void assign_move(ring_buffer& o) {
        size_type this_capacity = capacity_;
        size_type o_capacity = o.capacity_;
        size_type this_size = size();
        size_type o_size = o.size();

        //strong guarantee
        if (this_size == 0 || o_size == 0) {
          if (o_size != 0) {
            cleanup_and_move_source(o);
          }
          else {
            if (o_capacity != capacity_)
            {
              cleanup();
              allocate_buffer(o_capacity);
            }
          }
          return;
        }
        //strong guarantee
        if (o_capacity > capacity_)
        {
          cleanup_and_move_source(o);
          return;
        }
        //basic guarantee
        if constexpr (internal::is_nothrow_move_assignable_n_v<allocator_type, pointer>) {
          assign_move_helper(o, internal::move_tag{});
        }
        else {
          //basic guarantee
          if constexpr (internal::is_nothrow_copy_assignable_n_v<allocator_type, pointer>) {
            assign_move_helper(o, internal::copy_tag{});
          }
          //strong guarantee
          else {
            //destroy elements in current buffer and uninitialized_move from source with strong guarantee
            destroy_and_move_source(o);
            return;
          }
        }
      }

      CONSTEXPR void allocate_buffer(size_type count) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& start = data.data_;
        auto& first = data.last_;
        auto& last = data.last_;
        start = alloc.allocate(count);
        first = start;
        last = start;
        capacity_ = count;
      }

      CONSTEXPR void deallocate_buffer() {
        auto& alloc = get_allocator();
        auto& data = pair_.second_;
        auto& first = data.data_;
        auto& last = data.last_;
        alloc.deallocate(first, capacity_);
        first = nullptr;
        last = nullptr;
        capacity_ = 0;
      }

      CONSTEXPR void destroy_range(pointer first, pointer last) noexcept(std::is_nothrow_destructible_v<value_type>) {
        if constexpr (std::is_trivially_destructible_v<value_type>) {
          return;
        }
        else {
          if (first == last) return;

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

      CONSTEXPR void destroy_n(pointer first, size_type n) noexcept(std::is_nothrow_destructible_v<value_type>) {
        if constexpr (std::is_trivially_destructible_v<value_type>) {
          return;
        }
        else {
          if (n == 0) return;
          auto& alloc = pair_.first();
          auto& data = pair_.second_;
          pointer start = data.data_;
          pointer end = start + capacity_;

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

      struct storage {
        storage() noexcept : data_(nullptr), last_(nullptr), head_(0), tail_(0) {};
        storage(storage&& o) noexcept : data_(nullptr), last_(nullptr), head_(0), tail_(0) {};
        storage(const storage& o) noexcept : data_(nullptr), last_(nullptr), head_(o.head_), tail_(o.tail_) {};
        storage& operator=(const storage& o) {
          if (this == std::addressof(o)) return *this;
          head_ = o.head_;
          tail_ = o.tail_;
          return *this;
        };
        storage& operator=(storage&& o) noexcept {
          if (this == std::addressof(o)) return *this;
          data_ = std::exchange(o.data_, nullptr);
          last_ = std::exchange(o.last_, nullptr);
          head_ = std::exchange(o.head_, 0);
          tail_ = std::exchange(o.tail_, 0);
          return *this;
        };

        CONSTEXPR void swap(storage& o) noexcept {
          data_ = std::exchange(o.data_, data_);
          last_ = std::exchange(o.last_, last_);
          head_ = std::exchange(o.head_, head_);
          tail_ = std::exchange(o.tail_, tail_);
        };
        // start of array
        pointer data_;
        // first constructed element
        pointer first_;
        // last constructed element
        pointer last_;
        // current head pos
        size_type head_;
        // current tail pos
        size_type tail_;
      };

      utility::compressed_pair<allocator_type, storage> pair_;
      size_type capacity_;
      bool allow_overwrite_;
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

      explicit spsc_ring_buffer(size_type capacity_power_of_two, Alloc = {});
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