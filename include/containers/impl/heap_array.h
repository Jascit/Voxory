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
        CONSTEXPR ConstHeapArrayIterator(const container_base* container) noexcept : iterator_base(container), ptr_(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(const container_base* container, pointer p) noexcept : iterator_base(container), ptr_(p) {}
        ~ConstHeapArrayIterator() {
          release();
        }
#else
        CONSTEXPR ConstHeapArrayIterator() noexcept : ptr_(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(pointer p) noexcept : ptr_(p) {}
#endif // DEBUG_ITERATORS

        NODISCARD CONSTEXPR reference operator*() const noexcept { return *ptr_; }
        NODISCARD CONSTEXPR pointer operator->() const noexcept { return ptr_; }

        CONSTEXPR ConstHeapArrayIterator& operator++() noexcept {
          ++ptr_;
          return *this;
        }
        CONSTEXPR ConstHeapArrayIterator operator++(int) noexcept {
          ConstHeapArrayIterator tmp = *this;
          ++(*this);
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator--() noexcept {
          --ptr_;
          return *this;
        }
        CONSTEXPR ConstHeapArrayIterator operator--(int) noexcept {
          ConstHeapArrayIterator tmp = *this;
          --(*this);
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator+=(difference_type off) noexcept {
          ptr_ += off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstHeapArrayIterator operator+(difference_type off) const noexcept {
          ConstHeapArrayIterator tmp = *this;
          tmp += off;
          return tmp;
        }

        CONSTEXPR ConstHeapArrayIterator& operator-=(difference_type off) noexcept {
          ptr_ -= off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstHeapArrayIterator operator-(difference_type off) const noexcept {
          ConstHeapArrayIterator tmp = *this;
          tmp -= off;
          return tmp;
        }

        CONSTEXPR difference_type operator-(const ConstHeapArrayIterator& other) const noexcept {
          return static_cast<difference_type>(ptr_ - other.ptr_);
        }

        CONSTEXPR bool operator==(const ConstHeapArrayIterator& other) const noexcept { return ptr_ == other.ptr_; }
        CONSTEXPR bool operator!=(const ConstHeapArrayIterator& other) const noexcept { return ptr_ != other.ptr_; }
        CONSTEXPR bool operator<(const ConstHeapArrayIterator& other) const noexcept { return ptr_ < other.ptr_; }
        CONSTEXPR bool operator>(const ConstHeapArrayIterator& other) const noexcept { return ptr_ > other.ptr_; }
        CONSTEXPR bool operator<=(const ConstHeapArrayIterator& other) const noexcept { return ptr_ <= other.ptr_; }
        CONSTEXPR bool operator>=(const ConstHeapArrayIterator& other) const noexcept { return ptr_ >= other.ptr_; }

        NODISCARD CONSTEXPR reference operator[](difference_type off) const noexcept {
          return *(ptr_ + off);
        }

        pointer ptr_;
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
          return const_cast<pointer>(this->ptr_);
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

      heap_array(const allocator_type& alloc = allocator_type()) : pair_(FirstOneSecondArgs{}, alloc), capacity_(0) {}

      heap_array(size_t count, const value_type& val, const allocator_type& alloc = allocator_type())
        noexcept(noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : pair_(FirstOneSecondArgs{}, alloc), capacity_(count) {
        construct_range(count, val);
      };

      heap_array(size_t count, const allocator_type& alloc = allocator_type())
        noexcept(noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : pair_(FirstOneSecondArgs{}, alloc), capacity_(count)
      {
        construct_range(count);
      };

      heap_array(const heap_array& o) noexcept(std::is_nothrow_constructible_v<allocator_type, decltype(std::allocator_traits<allocator_type>::select_on_container_copy_construction(std::declval<allocator_type&>()))>) :
        pair_(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o.pair_.first())), capacity_(o.capacity_) {
        construct_range(o.capacity_, o.pair_.second_.first_, o.pair_.second_.last_);
      };

      heap_array(heap_array&& o) noexcept(std::is_nothrow_move_constructible_v<allocator_type>) : pair_(FirstOneSecondArgs{}, std::move(o.pair_.first()), std::move(o.pair_.second_)), capacity_(o.capacity_) {
        o.capacity_ = 0;
      };

      ~heap_array() {
        cleanup();
      }

      heap_array& operator=(heap_array&& o)
        noexcept(
          allocator_traits::propagate_on_container_move_assignment::value
          ? noexcept(get_allocator() = std::move(o.get_allocator()))
          : noexcept(assign(std::declval<heap_array&>(), internal::move_tag{}))
          )
      {
        if (this == &o) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
          cleanup();
          al = std::move(o_al);
          return steal_no_cleanup(o);
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            cleanup();
            return steal_no_cleanup(o);
          }
          else {
            if (al == o_al)
            {
              cleanup();
              return steal_no_cleanup(o);
            }
            else
            {
              assign(o, internal::move_tag{});
              o.cleanup();
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
            cleanup();
            al = o_al;
          }
        }

        assign(o, internal::copy_tag{});
        return *this;
      };

      reference operator[](size_t idx) noexcept {
#ifdef DEBUG_ITERATORS
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
#endif // DEBUG_ITERATORS

        return pair_.second_.first_[idx];
      };

      const_reference operator[](size_t idx) const noexcept {
#ifdef DEBUG
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
#endif // DEBUG_ITERATORS
        return pair_.second_.first_[idx];
      };

      NODISCARD CONSTEXPR pointer data() const noexcept {
        return pair_.second_.first_;
      };

      NODISCARD CONSTEXPR size_type capacity() const noexcept {
        return capacity_;
      };

      NODISCARD CONSTEXPR allocator_type& get_allocator() noexcept {
        return pair_.first();
      };

      NODISCARD CONSTEXPR const allocator_type& get_allocator() const noexcept {
        return pair_.first();
      };

      NODISCARD CONSTEXPR size_type size() const noexcept {
        return static_cast<size_type>(pair_.second_.last_ - pair_.second_.first_);
      };

      NODISCARD CONSTEXPR bool empty() const noexcept {
        return size() == 0;
      };

      CONSTEXPR void clear() noexcept {
        cleanup();
      };

      CONSTEXPR void resize(size_type size) noexcept(noexcept(resize_buffer(std::declval<size_type>()))) {
        resize_buffer(size);
      };

      CONSTEXPR void reserve(size_t size) noexcept(noexcept(reallocate(std::declval<size_type>()))) {
        if (size > capacity_)
        {
          reallocate(size);
        }
      };

      NODISCARD CONSTEXPR reference at(size_t index) {
        if (index >= size()) throw std::out_of_range("index out of range");
        return pair_.second_.first_[index];
      }

      NODISCARD CONSTEXPR const_reference at(size_t index) const {
        if (index >= size()) throw std::out_of_range("index out of range");
        return pair_.second_.first_[index];
      }

      NODISCARD CONSTEXPR iterator begin() noexcept {
        //return iterator(pair_.second_.first_);
        return ITER_DEBUG_WRAP(iterator, pair_.second_.first_);
      };

      NODISCARD CONSTEXPR iterator end() noexcept {
        return ITER_DEBUG_WRAP(iterator, pair_.second_.last_);
      };

      NODISCARD CONSTEXPR const_iterator end() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.last_);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.first_);
      };

      NODISCARD CONSTEXPR const_iterator cend() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.last_);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.first_);
      };

      CONSTEXPR void shrink_to_fit() {
        size_type current_size = size();
        if (current_size < capacity_) {
          if (current_size == 0) {
            cleanup();
            return;
          }
          reallocate(current_size);
        }
      }

    private:
      CONSTEXPR inline heap_array& steal_no_cleanup(heap_array& o) noexcept(
#ifdef DEBUG_ITERATORS
        noexcept(move_proxies(std::declval<heap_array&>())) &&
#endif // DEBUG_ITERATORS
        true) {
        pair_.second_ = std::move(o.pair_.second_);
        capacity_ = std::exchange(o.capacity_, 0);
#ifdef DEBUG_ITERATORS
        move_proxies(o);
#endif // DEBUG_ITERATORS
        return *this;
        };

      CONSTEXPR void reallocate(size_type new_capacity) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first_;
        auto& last = data.last_;

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

        cleanup();

        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
      };

      CONSTEXPR void resize_reallocate(size_type new_capacity) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first_;
        auto& last = data.last_;

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

        cleanup();

        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
      };

      CONSTEXPR void resize_buffer(size_type new_size) {
        size_type old_size = size();
        if (new_size == old_size) return;

        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first_;
        auto& last = data.last_;


        if (new_size > capacity_) {
          resize_reallocate(new_size);
          return;
        }
        if (new_size < old_size) {
          size_type to_destroy = old_size - new_size;
          pointer new_last = last - to_destroy;
          destroy_range(new_last, last);
          last = new_last;
#ifdef DEBUG_ITERATORS
          invalidate_range(first, last);
#endif // DEBUG_ITERATORS
        }
        else if (new_size > old_size) {
          size_type to_construct = new_size - old_size;
          pointer new_last = last + to_construct;
          last = internal::uninitialized_default_construct(last, new_last, alloc);
#ifdef DEBUG_ITERATORS
          invalidate_range(first, last);
#endif // DEBUG_ITERATORS
        }
        return;
      };

      // tag to select between copy and move
      template<typename Tag>
      CONSTEXPR void clean_and_create_buffer_src(size_type new_capacity, pointer src, size_type new_size, Tag&&) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first_;
        auto& last = data.last_;

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

        cleanup();

        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
      };

      CONSTEXPR void cleanup() noexcept(noexcept(deallocate_buffer())) {
        auto& data = pair_.second_;
        auto& first = data.first_;
        auto& last = data.last_;
#ifdef DEBUG_ITERATORS
       release_proxy();
#endif // DEBUG_ITERATORS
        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
        }
      };

      CONSTEXPR void allocate_buffer(size_t n) noexcept(noexcept(std::declval<allocator_type&>().allocate(std::declval<size_t>()))) {
        auto& alloc = get_allocator();
        auto& data = pair_.second_;
        pointer new_data = alloc.allocate(n);
        data.first_ = new_data;
        data.last_ = new_data;
        capacity_ = n;
      };

      CONSTEXPR void deallocate_buffer() noexcept(noexcept(std::declval<allocator_type&>().deallocate(std::declval<pointer>(), std::declval<size_type>()))) {
        auto& alloc = get_allocator();
        auto& data = pair_.second_;
        auto& first = data.first_;
        auto& last = data.last_;
        alloc.deallocate(first, capacity_);
        first = nullptr;
        last = nullptr;
        capacity_ = 0;
      };

      CONSTEXPR void destroy_range(pointer first, pointer end) noexcept {
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
      CONSTEXPR void construct_range(size_t count, Args&&... args) noexcept(detail::IsNoexceptConstructRange_v<pointer, allocator_type, Args...>) {
        // sizeof...(Args) == 0 = default construct
        // sizeof...(Args) == 1 = fill_n with value
        // sizeof...(Args) == 2 = copy construct from src(start, end)
        auto& data = pair_.second_;
        auto& alloc = get_allocator();

        auto& first = data.first_;
        auto& last = data.last_;

        constexpr uint8_t args_count = sizeof...(Args);
        if (count != 0)
        {
          allocate_buffer(count);
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
      CONSTEXPR void assign(const heap_array& o, Tag&& tag) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();

        auto& first = data.first_;
        auto& last = data.last_;

        auto& o_data = o.pair_.second_;
        auto& o_first = o_data.first_;

        size_type o_cap = o.capacity();
        size_type o_size = o.size();
        size_type size = static_cast<size_type>(last - first); // count of alive objects & default constructed 

        // allocate new buffer, copy/move-construct all
        if (o_size > capacity_) {
          clean_and_create_buffer_src(o_cap, o_first, o_size, tag);
          return;
        }

        // copy/move-assign within existing buffer, copy-construct rest
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
          invalidate_range(first, last);
#endif // DEBUG_ITERATORS
          return;
        }
        // copy/move-assign within existing buffer, destroy rest
        else
        {
          size_type common = o_size;
          size_type rest = size - o_size;
          if constexpr (std::is_same_v<internal::copy_tag, Tag>) {
            pointer mid = internal::copy_assign_n(o_first, common, first, alloc);
            destroy_range(mid, mid + rest);
            last = mid;
          }
          else {
            pointer mid = internal::move_assign_n(o_first, common, first, alloc);
            destroy_range(mid, mid + rest);
            last = mid;
          }
#ifdef DEBUG_ITERATORS
          if (rest != 0) {
            invalidate_range(first, last);
          }
#endif // DEBUG_ITERATORS
          return;
        }
      }
#ifdef DEBUG_ITERATORS
      CONSTEXPR void invalidate_range(pointer first, pointer last) noexcept {
        lock();
        list::node_type* current = list_.data();
        while (current != nullptr)
        {
          list::node_type* next = current->next_;
          if (reinterpret_cast<const_iterator*>(current->data_)->ptr_ < first || reinterpret_cast<const_iterator*>(current->data_)->ptr_ > last)
          {
            current->data_->proxy_ = nullptr;
            list_.pop_at(current);
          }
          current = next;
        }
        unlock();
      }
#endif // DEBUG_ITERATORS
    protected:
      struct storage {
        storage(pointer first, pointer last) : first_(first), last_(last) {};
        storage() : first_(nullptr), last_(nullptr) {};
        storage(const storage& o) = delete;
        storage(storage&& o) noexcept : first_(std::exchange(o.first_, nullptr)), last_(std::exchange(o.last_, nullptr)) {};

        storage& operator=(const storage&) = delete;
        storage& operator=(storage&& o) noexcept {
          if (this == std::addressof(o)) return *this;
          first_ = std::exchange(o.first_, nullptr);
          last_ = std::exchange(o.last_, nullptr);
          return *this;
        };

        CONSTEXPR void swap(storage& o) noexcept {
          first_ = std::exchange(o.first_, first_);
          last_ = std::exchange(o.last_, last_);
        };

        pointer first_;
        pointer last_;
      };
      utility::compressed_pair<allocator_type, storage> pair_;
      size_type capacity_;
    };
  }
}