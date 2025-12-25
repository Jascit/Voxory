#pragma once
#define NOMINMAX
#include <utility.h>
#include <platform/platform.h>
#include <stdexcept>
#include <containers/detail/containers_internal.h>
#include <containers/containers_dbg.h>

namespace voxory {
  namespace containers {
    namespace detail {
      template<typename Pointer, typename Alloc, typename... Args>
      concept IsNoexceptConstructRange =
        (sizeof...(Args) == 0 &&
          noexcept(internal::uninitialized_default_construct(
            std::declval<Pointer>(), std::declval<Pointer>(), std::declval<Alloc&>())))
        ||
        (sizeof...(Args) == 1 &&
          noexcept(internal::uninitialized_fill_n(
            std::declval<Pointer>(), std::declval<size_t>(),
            std::declval<Args>()..., std::declval<Alloc&>())))
        ||
        (sizeof...(Args) == 2 &&
          noexcept(internal::uninitialized_copy(
            std::declval<Args>()..., std::declval<Pointer>(), std::declval<Alloc&>())));

      template<typename Pointer, typename Alloc, typename... Args>
      inline constexpr bool IsNoexceptConstructRange_v = IsNoexceptConstructRange<Pointer, Alloc, Args...>;

      template<typename Traits>
      class ConstHeapArrayIterator
#ifdef DEBUG_ITERATORS
        : iterator_base
#endif // DEBUG_ITERATORS
      {
      public:
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::const_pointer;
        using reference = typename Traits::const_reference;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
#ifdef DEBUG_ITERATORS
        CONSTEXPR ConstHeapArrayIterator(const container_base* container) noexcept : iterator_base(container), _ptr(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(const container_base* container, pointer p) noexcept : iterator_base(container), _ptr(p) {}
        ~ConstHeapArrayIterator() {
          _release();
        }
#else
        CONSTEXPR ConstHeapArrayIterator() noexcept : _ptr(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(pointer p) noexcept : _ptr(p) {}
#endif // DEBUG_ITERATORS

        NODISCARD CONSTEXPR reference operator*() const noexcept { return *_ptr; }
        NODISCARD CONSTEXPR pointer operator->() const noexcept { return _ptr; }

        CONSTEXPR ConstHeapArrayIterator& operator++() noexcept {
          ++_ptr;
          return *this;
        }
        CONSTEXPR ConstHeapArrayIterator operator++(int) noexcept {
          ConstHeapArrayIterator tmp = *this;
          ++(*this);
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator--() noexcept {
          --_ptr;
          return *this;
        }
        CONSTEXPR ConstHeapArrayIterator operator--(int) noexcept {
          ConstHeapArrayIterator tmp = *this;
          --(*this);
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator+=(difference_type off) noexcept {
          _ptr += off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstHeapArrayIterator operator+(difference_type off) const noexcept {
          ConstHeapArrayIterator tmp = *this;
          tmp += off;
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator-=(difference_type off) noexcept {
          _ptr -= off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstHeapArrayIterator operator-(difference_type off) const noexcept {
          ConstHeapArrayIterator tmp = *this;
          tmp -= off;
          return tmp;
        }

        CONSTEXPR difference_type operator-(const ConstHeapArrayIterator& other) const noexcept {
          return static_cast<difference_type>(_ptr - other._ptr);
        }

        CONSTEXPR bool operator==(const ConstHeapArrayIterator& other) const noexcept { return _ptr == other._ptr; }
        CONSTEXPR bool operator!=(const ConstHeapArrayIterator& other) const noexcept { return _ptr != other._ptr; }
        CONSTEXPR bool operator<(const ConstHeapArrayIterator& other) const noexcept { return _ptr < other._ptr; }
        CONSTEXPR bool operator>(const ConstHeapArrayIterator& other) const noexcept { return _ptr > other._ptr; }
        CONSTEXPR bool operator<=(const ConstHeapArrayIterator& other) const noexcept { return _ptr <= other._ptr; }
        CONSTEXPR bool operator>=(const ConstHeapArrayIterator& other) const noexcept { return _ptr >= other._ptr; }

        NODISCARD CONSTEXPR reference operator[](difference_type off) const noexcept {
          return *(_ptr + off);
        }

        pointer _ptr;
      };

      template<typename Traits>
      class HeapArrayIterator : public ConstHeapArrayIterator<Traits> {
        using base_type = ConstHeapArrayIterator<Traits>;
      public:
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::pointer;
        using reference = typename Traits::reference;
        using difference_type = typename base_type::difference_type;
        using iterator_category = std::random_access_iterator_tag;
#ifdef DEBUG_ITERATORS
        CONSTEXPR HeapArrayIterator(container_base* container) noexcept : base_type(container, nullptr) {}
        CONSTEXPR explicit HeapArrayIterator(container_base* container, pointer p) noexcept : base_type(container, p) {}
        ~HeapArrayIterator() = default;
#else
        CONSTEXPR HeapArrayIterator() noexcept : base_type(nullptr) {}
        CONSTEXPR explicit HeapArrayIterator(pointer p) noexcept : base_type(p) {}
#endif // DEBUG_ITERATORS


        NODISCARD CONSTEXPR reference operator*() const noexcept {
          return const_cast<reference>(base_type::operator*());
        }
        NODISCARD CONSTEXPR pointer operator->() const noexcept {
          return const_cast<pointer>(this->_ptr);
        }

        CONSTEXPR HeapArrayIterator& operator++() noexcept {
          base_type::operator++();
          return *this;
        }
        CONSTEXPR HeapArrayIterator operator++(int) noexcept {
          HeapArrayIterator tmp = *this;
          base_type::operator++();
          return tmp;
        }

        CONSTEXPR HeapArrayIterator& operator--() noexcept {
          base_type::operator--();
          return *this;
        }
        CONSTEXPR HeapArrayIterator operator--(int) noexcept {
          HeapArrayIterator tmp = *this;
          base_type::operator--();
          return tmp;
        }

        CONSTEXPR HeapArrayIterator& operator+=(difference_type off) noexcept {
          base_type::operator+=(off);
          return *this;
        }
        CONSTEXPR HeapArrayIterator& operator-=(difference_type off) noexcept {
          base_type::operator-=(off);
          return *this;
        }

        NODISCARD CONSTEXPR HeapArrayIterator operator+(difference_type off) const noexcept {
          HeapArrayIterator tmp = *this;
          tmp += off;
          return tmp;
        }
        NODISCARD CONSTEXPR HeapArrayIterator operator-(difference_type off) const noexcept {
          HeapArrayIterator tmp = *this;
          tmp -= off;
          return tmp;
        }

        CONSTEXPR difference_type operator-(const HeapArrayIterator& other) const noexcept {
          return base_type::operator-(other);
        }

        CONSTEXPR bool operator==(const HeapArrayIterator& other) const noexcept { return base_type::operator==(other); }
        CONSTEXPR bool operator!=(const HeapArrayIterator& other) const noexcept { return base_type::operator!=(other); }

        NODISCARD CONSTEXPR reference operator[](difference_type off) const noexcept {
          return const_cast<reference>(base_type::operator[](off));
        }
      };
    }

    template<typename T, typename Alloc = std::allocator<T>>
    class heap_array
#ifdef DEBUG_ITERATORS
      : container_base
#endif // DEBUG
    {
      using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
      using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
    public:
      using value_type = T;
      using allocator_type = Alloc;
      using allocator_traits = std::allocator_traits<allocator_type>;
      using reference = value_type&;
      using const_reference = const value_type&;
      using pointer = typename allocator_traits::pointer;
      using const_pointer = typename allocator_traits::const_pointer;
      using size_type = typename allocator_traits::size_type;
      using cleanup_guard = internal::cleanup_guard<heap_array>;
      using trivial_traits = type_traits::trivial_traits<value_type>;
      using iterator = detail::HeapArrayIterator<heap_array>;
      using const_iterator = detail::ConstHeapArrayIterator<heap_array>;
      friend cleanup_guard;

      static_assert(std::is_same_v<value_type, typename allocator_traits::value_type>, "value_type must match allocator_traits::value_type");
      static_assert(std::is_object_v<value_type>, "value_type must be an object type");

      heap_array(const allocator_type& alloc = allocator_type()) : _pair(FirstOneSecondArgs{}, alloc), _capacity(0) {}

      heap_array(size_t count, const value_type& val, const allocator_type& alloc = allocator_type())
        noexcept(noexcept(_construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : _pair(FirstOneSecondArgs{}, alloc), _capacity(count) {
        _construct_range(count, val);
      };

      heap_array(size_t count, const allocator_type& alloc = allocator_type())
        noexcept(noexcept(_construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : _pair(FirstOneSecondArgs{}, alloc), _capacity(count)
      {
        _construct_range(count);
      };

      heap_array(const heap_array& o) noexcept(std::is_nothrow_constructible_v<allocator_type, decltype(std::allocator_traits<allocator_type>::select_on_container_copy_construction(std::declval<allocator_type&>()))>) :
        _pair(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o._pair.first())), _capacity(o._capacity) {
        _construct_range(o._capacity, o._pair._second._first, o._pair._second._last);
      };

      heap_array(heap_array&& o) noexcept(std::is_nothrow_move_constructible_v<allocator_type>) : _pair(FirstOneSecondArgs{}, std::move(o._pair.first()), std::move(o._pair._second)), _capacity(o._capacity) {
        o._capacity = 0;
      };

      ~heap_array() {
        _cleanup();
      }

      heap_array& operator=(heap_array&& o)
        noexcept(
          allocator_traits::propagate_on_container_move_assignment::value
          ? noexcept(get_allocator() = std::move(o.get_allocator()))
          : noexcept(_assign(std::declval<heap_array&>(), internal::move_tag{}))
          )
      {
        if (this == &o) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
          _cleanup();
          al = std::move(o_al);
          return _steal_no_cleanup(o);
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            _cleanup();
            return _steal_no_cleanup(o);
          }
          else {
            if (al == o_al)
            {
              _cleanup();
              return _steal_no_cleanup(o);
            }
            else
            {
              _assign(o, internal::move_tag{});
              o._cleanup();
              return *this;
            }
          }
        }
      };

      heap_array& operator=(const heap_array& o) {
        if (this == &o) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();


        if constexpr (allocator_traits::propagate_on_container_copy_assignment::value) {
          //change allocator, allocate with copy ctor from new one
          if (al != o_al) {
            _cleanup();
            al = o_al;
          }
        }

        _assign(o, internal::copy_tag{});
        return *this;
      };

      reference operator[](size_t idx) noexcept {
#ifdef DEBUG_ITERATORS
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
#endif // DEBUG_ITERATORS

        return _pair._second._first[idx];
      };

      const_reference operator[](size_t idx) const noexcept {
#ifdef DEBUG
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
#endif // DEBUG_ITERATORS
        return _pair._second._first[idx];
      };

      NODISCARD CONSTEXPR pointer data() const noexcept {
        return _pair._second._first;
      };

      NODISCARD CONSTEXPR size_type capacity() const noexcept {
        return _capacity;
      };

      NODISCARD CONSTEXPR allocator_type& get_allocator() noexcept {
        return _pair.first();
      };

      NODISCARD CONSTEXPR const allocator_type& get_allocator() const noexcept {
        return _pair.first();
      };

      NODISCARD CONSTEXPR size_type size() const noexcept {
        return static_cast<size_type>(_pair._second._last - _pair._second._first);
      };

      NODISCARD CONSTEXPR bool empty() const noexcept {
        return size() == 0;
      };

      CONSTEXPR void clear() noexcept {
        _cleanup();
      };

      CONSTEXPR void resize(size_type size) noexcept(noexcept(_resize_buffer(std::declval<size_type>()))) {
        _resize_buffer(size);
      };

      CONSTEXPR void reserve(size_t size) noexcept(noexcept(_reallocate(std::declval<size_type>()))) {
        if (size > _capacity)
        {
          _reallocate(size);
        }
      };

      NODISCARD CONSTEXPR reference at(size_t index) {
        if (index >= size()) throw std::out_of_range("index out of range");
        return _pair._second._first[index];
      }

      NODISCARD CONSTEXPR const_reference at(size_t index) const {
        if (index >= size()) throw std::out_of_range("index out of range");
        return _pair._second._first[index];
      }

      NODISCARD CONSTEXPR iterator begin() noexcept {
        //return iterator(_pair._second._first);
        return ITER_DEBUG_WRAP(iterator, _pair._second._first);
      };

      NODISCARD CONSTEXPR iterator end() noexcept {
        return ITER_DEBUG_WRAP(iterator, _pair._second._last);
      };

      NODISCARD CONSTEXPR const_iterator end() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, _pair._second._last);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, _pair._second._first);
      };

      NODISCARD CONSTEXPR const_iterator cend() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, _pair._second._last);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, _pair._second._first);
      };

      CONSTEXPR void shrink_to_fit() {
        size_type current_size = size();
        if (current_size < _capacity) {
          if (current_size == 0) {
            _cleanup();
            return;
          }
          _reallocate(current_size);
        }
      }

    private:
      CONSTEXPR inline heap_array& _steal_no_cleanup(heap_array& o) noexcept(
#ifdef DEBUG_ITERATORS
        noexcept(_move_proxies(std::declval<heap_array&>())) &&
#endif // DEBUG_ITERATORS
        true) {
        _pair._second = std::move(o._pair._second);
        _capacity = std::exchange(o._capacity, 0);
#ifdef DEBUG_ITERATORS
        _move_proxies(o);
#endif // DEBUG_ITERATORS
        return *this;
        };

      CONSTEXPR void _reallocate(size_type new_capacity) {
        auto& data = _pair._second;
        auto& alloc = get_allocator();
        auto& first = data._first;
        auto& last = data._last;

        pointer new_data = alloc.allocate(new_capacity);
        pointer new_last = new_data;
        internal::realloc_guard<allocator_type> guard(alloc, new_data, new_capacity);
        if (first != nullptr)
        {
          //guard, if exception occurs during move: destroy new_data, leave old intact
          if constexpr (noexcept(internal::uninitialized_move(std::declval<pointer>(), std::declval<pointer>(), std::declval<pointer>(), std::declval<allocator_type&>())))
          {
            new_last = internal::uninitialized_move(first, last, new_data, alloc);
          }
          else
          {
            new_last = internal::uninitialized_copy(first, last, new_data, alloc);
          }
        }
        guard.release();

        _cleanup();

        first = new_data;
        last = new_last;
        _capacity = new_capacity;
      };

      CONSTEXPR void _resize_reallocate(size_type new_capacity) {
        auto& data = _pair._second;
        auto& alloc = get_allocator();
        auto& first = data._first;
        auto& last = data._last;

        pointer new_data = alloc.allocate(new_capacity);
        pointer new_last = new_data;
        internal::realloc_guard<allocator_type> guard(alloc, new_data, new_capacity);
        if (first != nullptr)
        {
          //guard, if exception occurs during move: destroy new_data, leave old intact
          if constexpr (noexcept(internal::uninitialized_move(std::declval<pointer>(), std::declval<pointer>(), std::declval<pointer>(), std::declval<allocator_type&>())))
          {
            new_last = internal::uninitialized_move(first, last, new_data, alloc);
          }
          else
          {
            new_last = internal::uninitialized_copy(first, last, new_data, alloc);
          }
        }
        size_type old_size = static_cast<size_type>(last - first);
        size_type to_construct = new_capacity > old_size ? (new_capacity - old_size) : 0;
        new_last = internal::uninitialized_default_construct_n(new_last, to_construct, alloc);
        guard.release();

        _cleanup();

        first = new_data;
        last = new_last;
        _capacity = new_capacity;
      };

      CONSTEXPR void _resize_buffer(size_type new_size) {
        size_type old_size = size();
        if (new_size == old_size) return;

        auto& data = _pair._second;
        auto& alloc = get_allocator();
        auto& first = data._first;
        auto& last = data._last;


        if (new_size > _capacity) {
          _resize_reallocate(new_size);
          return;
        }
        if (new_size < old_size) {
          size_type to_destroy = old_size - new_size;
          pointer new_last = last - to_destroy;
          _destroy_range(new_last, last);
          last = new_last;
#ifdef DEBUG_ITERATORS
          _invalidate_range(first, last);
#endif // DEBUG_ITERATORS
        }
        else if (new_size > old_size) {
          size_type to_construct = new_size - old_size;
          pointer new_last = last + to_construct;
          last = internal::uninitialized_default_construct(last, new_last, alloc);
#ifdef DEBUG_ITERATORS
          _invalidate_range(first, last);
#endif // DEBUG_ITERATORS
        }
        return;
      };

      // tag to select between copy and move
      template<typename Tag>
      CONSTEXPR void _clean_and_create_buffer_src(size_type new_capacity, pointer src, size_type new_size, Tag&&) {
        auto& data = _pair._second;
        auto& alloc = get_allocator();
        auto& first = data._first;
        auto& last = data._last;

        pointer new_data = alloc.allocate(new_capacity);
        pointer new_last = new_data;

        //guard, if exception occurs during move/copy: destroy new_data, leave old intact 
        internal::realloc_guard<allocator_type> guard(alloc, new_data, new_capacity);
        if constexpr (std::is_same_v<internal::copy_tag, Tag>) {
          new_last = internal::uninitialized_copy(src, src + new_size, new_data, alloc);
        }
        else
        {
          new_last = internal::uninitialized_move(src, src + new_size, new_data, alloc);
        }
        guard.release();

        _cleanup();

        first = new_data;
        last = new_last;
        _capacity = new_capacity;
      };

      CONSTEXPR void _cleanup() noexcept(noexcept(_deallocate_buffer())) {
        auto& data = _pair._second;
        auto& first = data._first;
        auto& last = data._last;
#ifdef DEBUG_ITERATORS
       _release_proxy();
#endif // DEBUG_ITERATORS
        if (first != nullptr)
        {
          _destroy_range(first, last);
          _deallocate_buffer();
        }
      };

      CONSTEXPR void _allocate_buffer(size_t n) noexcept(noexcept(std::declval<allocator_type&>().allocate(std::declval<size_t>()))) {
        auto& alloc = get_allocator();
        auto& data = _pair._second;
        pointer new_data = alloc.allocate(n);
        data._first = new_data;
        data._last = new_data;
        _capacity = n;
      };

      CONSTEXPR void _deallocate_buffer() noexcept(noexcept(std::declval<allocator_type&>().deallocate(std::declval<pointer>(), std::declval<size_type>()))) {
        auto& alloc = get_allocator();
        auto& data = _pair._second;
        auto& first = data._first;
        auto& last = data._last;
        alloc.deallocate(first, _capacity);
        first = nullptr;
        last = nullptr;
        _capacity = 0;
      };

      CONSTEXPR void _destroy_range(pointer first, pointer end) noexcept {
        if constexpr (!trivial_traits::is_trivially_destructible::value) {
          auto& alloc = get_allocator();
          for (; first < end; first++) {
            if constexpr (type_traits::has_destroy<allocator_type, pointer>::value) {
              alloc.destroy(first);
            }
            else {
              std::destroy_at(first);
            }
          }
        }
      };

      template<typename... Args>
      CONSTEXPR void _construct_range(size_t count, Args&&... args) noexcept(detail::IsNoexceptConstructRange_v<pointer, allocator_type, Args...>) {
        // sizeof...(Args) == 0 = default construct
        // sizeof...(Args) == 1 = fill_n with value
        // sizeof...(Args) == 2 = copy construct from src(start, end)
        auto& data = _pair._second;
        auto& alloc = get_allocator();

        auto& first = data._first;
        auto& last = data._last;

        constexpr uint8_t args_count = sizeof...(Args);
        if (count != 0)
        {
          _allocate_buffer(count);
          cleanup_guard guard(this);
          if constexpr (args_count == 0) {
            last = internal::uninitialized_default_construct_n(first, count, alloc);
          }
          else if constexpr (args_count == 1) {
            last = internal::uninitialized_fill_n(first, count, std::forward<Args>(args)..., alloc);
          }
          else if constexpr (args_count == 2) {
            last = internal::uninitialized_copy(std::forward<Args>(args)..., first, alloc);
          }
          guard.release();
        }
      };

      template<typename Tag>
      CONSTEXPR void _assign(const heap_array& o, Tag&& tag) {
        auto& data = _pair._second;
        auto& alloc = get_allocator();

        auto& first = data._first;
        auto& last = data._last;

        auto& o_data = o._pair._second;
        auto& o_first = o_data._first;

        size_type o_cap = o.capacity();
        size_type o_size = o.size();
        size_type size = static_cast<size_type>(last - first); // count of alive objects & default constructed 
        
        if (size == 0 || o_size == 0) {
          if (o_size != 0) {
            _clean_and_create_buffer_src(o_cap, o_first, o_size, tag);
          }
          else {
            if (o_cap != _capacity)
            {
              _deallocate_buffer();
              _allocate_buffer(o_cap);
            }
          }
          return;
        }
        // allocate new buffer, copy/move-construct all
        if (o_size > _capacity) {
          _clean_and_create_buffer_src(o_cap, o_first, o_size, tag);
          return;
        }

        // copy/move-_assign within existing buffer, copy-construct rest
        if (o_size > size) {
          size_type common = size;
          size_type rest = o_size - size;
          if constexpr (std::is_same_v<internal::copy_tag, Tag>) {
            pointer mid = internal::copy_assign_n(o_first, common, first, alloc);
            mid = internal::uninitialized_copy_n(o_first + common, rest, mid, alloc);
            last = mid;
          }
          else {
            pointer mid = internal::move_assign_n(o_first, common, first, alloc);
            mid = internal::uninitialized_move_n(o_first + common, rest, mid, alloc);
            last = mid;
          }
#ifdef DEBUG_ITERATORS
          _invalidate_range(first, last);
#endif // DEBUG_ITERATORS
          return;
        }
        // copy/move-_assign within existing buffer, destroy rest
        else
        {
          size_type common = o_size;
          size_type rest = size - o_size;
          if constexpr (std::is_same_v<internal::copy_tag, Tag>) {
            pointer mid = internal::copy_assign_n(o_first, common, first, alloc);
            _destroy_range(mid, mid + rest);
            last = mid;
          }
          else {
            pointer mid = internal::move_assign_n(o_first, common, first, alloc);
            _destroy_range(mid, mid + rest);
            last = mid;
          }
#ifdef DEBUG_ITERATORS
          if (rest != 0) {
            _invalidate_range(first, last);
          }
#endif // DEBUG_ITERATORS
          return;
        }
      }
#ifdef DEBUG_ITERATORS
      CONSTEXPR void _invalidate_range(pointer first, pointer last) noexcept {
        _lock();
        list::node_type* current = _list.data();
        while (current != nullptr)
        {
          list::node_type* next = current->_next;
          if (reinterpret_cast<const_iterator*>(current->_data)->_ptr < first || reinterpret_cast<const_iterator*>(current->_data)->_ptr > last)
          {
            current->_data->_proxy = nullptr;
            _list.pop_at(current);
          }
          current = next;
        }
        _unlock();
      }
#endif // DEBUG_ITERATORS
    protected:
      struct _storage {
        _storage(pointer first, pointer last) : _first(first), _last(last) {};
        _storage() : _first(nullptr), _last(nullptr) {};
        _storage(const _storage& o) = delete;
        _storage(_storage&& o) noexcept : _first(std::exchange(o._first, nullptr)), _last(std::exchange(o._last, nullptr)) {};

        _storage& operator=(const _storage&) = delete;
        _storage& operator=(_storage&& o) noexcept {
          if (this == std::addressof(o)) return *this;
          _first = std::exchange(o._first, nullptr);
          _last = std::exchange(o._last, nullptr);
          return *this;
        };

        CONSTEXPR void swap(_storage& o) noexcept {
          _first = std::exchange(o._first, _first);
          _last = std::exchange(o._last, _last);
        };

        pointer _first;
        pointer _last;
      };
      utility::compressed_pair<allocator_type, _storage> _pair;
      size_type _capacity;
    };
  }
}