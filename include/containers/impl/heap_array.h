#pragma once
#define NOMINMAX
#include <utility.h>
#include <type_traits.h>
#include <stdexcept>
#include <platform/platform.h>
#include <containers/detail/containers_internal.h>
#include <containers/container_dbg.h>

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
#ifdef DEBUG
        : debug::iterator_base
#endif // DEBUG
      {
      public:
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::pointer;
        using const_pointer = typename Traits::const_pointer;
        using reference = typename Traits::const_reference;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
#ifdef DEBUG
        CONSTEXPR ConstHeapArrayIterator(const debug::container_base* container) noexcept : debug::iterator_base(container), ptr_(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(const debug::container_base* container, pointer p) noexcept : debug::iterator_base(container), ptr_(p) {}
        ~ConstHeapArrayIterator() {
          release_on_destroy();
        }
#else
        CONSTEXPR ConstHeapArrayIterator() noexcept : ptr_(nullptr) {}
        CONSTEXPR explicit ConstHeapArrayIterator(pointer p) noexcept : ptr_(p) {}
#endif // DEBUG

        NODISCARD CONSTEXPR reference operator*() const noexcept { return *ptr_; }
        NODISCARD CONSTEXPR const_pointer operator->() const noexcept { return ptr_; }

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

      protected:
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
#ifdef DEBUG
        CONSTEXPR HeapArrayIterator(debug::container_base* container) noexcept : base_type(container, nullptr) {}
        CONSTEXPR explicit HeapArrayIterator(debug::container_base* container, pointer p) noexcept : base_type(container, p) {}
        ~HeapArrayIterator() = default;
#else
        CONSTEXPR HeapArrayIterator() noexcept : base_type(nullptr) {}
        CONSTEXPR explicit HeapArrayIterator(pointer p) noexcept : base_type(p) {}
#endif // DEBUG


        NODISCARD CONSTEXPR reference operator*() const noexcept {
          return const_cast<reference>(base_type::operator*());
        }
        NODISCARD CONSTEXPR pointer operator->() const noexcept {
          return this->ptr_;
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

      
      struct move_tag {};
      struct copy_tag {};
      struct no_grow {
        static FORCE_INLINE NODISCARD CONSTEXPR std::size_t calculate_grow(std::size_t current_capacity) noexcept {
          return current_capacity;
        }
      };
      struct adaptive_growth {
        static NODISCARD CONSTEXPR std::size_t calculate_grow(std::size_t current_capacity) noexcept {
          if (current_capacity < 16)
            return current_capacity * 2;  // маленьк≥ масиви ростуть швидко
          else
            return current_capacity + (current_capacity / 4); // велик≥ Ч пов≥льн≥ше
        }
      };

    }
    // debug-friendly Policy дл€ п≥дстановки в шаблон
    template<
      typename T,
      typename Alloc = std::allocator<T>,
      typename Ptr = T*,
      typename Sz = std::size_t,
      typename GrowPolicy = detail::no_grow
    >
    struct debug_policy {
      using value_type = T;
      using pointer = Ptr;
      using const_pointer = const Ptr;
      using reference = T&;
      using const_reference = reference;
      using size_type = Sz;
      using allocator_type = Alloc;
      using allocator = Alloc;
      using grow_policy = GrowPolicy;
      using iterator = detail::HeapArrayIterator<debug_policy>;
      using const_iterator = detail::ConstHeapArrayIterator<debug_policy>;
    };

    template<typename Policy>
    class heap_array 
#ifdef DEBUG
      : debug::container_base 
#endif // DEBUG
    {
      using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
      using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
    public:
      using value_type = typename Policy::value_type;
      using pointer = typename Policy::pointer;
      using size_type = typename Policy::size_type;
      using cleanup_guard = internal::cleanup_guard<heap_array>;
      using allocator_type = internal::get_allocator_type<Policy>::type;
      using allocator_traits = std::allocator_traits<allocator_type>;
      using trivial_traits = type_traits::trivial_traits<value_type>;
      using grow_policy = typename Policy::grow_policy;
      using iterator = typename Policy::iterator;
      using const_iterator = typename Policy::const_iterator;

      static_assert(std::is_same_v<value_type, typename allocator_traits::value_type>, "value_type must match allocator_traits::value_type");
      static_assert(std::is_object_v<value_type>, "value_type must be an object type");

      heap_array(size_t count, value_type&& val, allocator_type& alloc = allocator_type())
        noexcept(noexcept(allocate_buffer_non_zero(std::declval<size_t>()))
          && noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : pair_(FirstOneSecondArgs{}, alloc), capacity_(count) {
        construct_range(count, std::forward<value_type>(val));
      };

      heap_array(size_t count, allocator_type& alloc = allocator_type())
        noexcept(noexcept(allocate_buffer_non_zero(std::declval<size_t>()))
          && noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
        : pair_(FirstOneSecondArgs{}, alloc), capacity_(count)
      {
        construct_range(count);
      };

      heap_array(const heap_array& o) noexcept(std::is_nothrow_constructible_v<allocator_type, decltype(std::allocator_traits<allocator_type>::select_on_container_copy_construction(std::declval<allocator_type&>()))>) :
        pair_(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o.pair_.first())), capacity_(o.capacity_) {
        construct_range(o.capacity_, o.pair_.second_.first, o.pair_.second_.second);
      };

      heap_array(heap_array&& o) noexcept(std::is_nothrow_move_constructible_v<allocator_type>) : pair_(FirstOneSecondArgs{}, std::move(o.pair_.first())), capacity_(o.capacity_) {
        pair_.second_.first = o.pair_.second_.first;
        pair_.second_.second = o.pair_.second_.second;
        o.pair_.second_.first = nullptr;
        o.pair_.second_.second = nullptr;
        o.capacity_ = 0;
      };

      ~heap_array() {
        cleanup();
      }

      heap_array& operator=(heap_array&& o)
        noexcept(
          allocator_traits::propagate_on_container_move_assignment::value
          ? noexcept(get_allocator() = std::move(o.get_allocator()))
          : noexcept(assign(std::declval<heap_array&>(), detail::move_tag{}))
          )
      {
        if (this == &o) return *this;
        auto& al = get_allocator();
        auto& o_al = o.get_allocator();

        auto& data = pair_.second_;
        auto& o_data = o.pair_.second_;

        auto steal_no_cleanup = [&]() -> heap_array& {
          data.first = std::exchange(o_data.first, nullptr);
          data.second = std::exchange(o_data.second, nullptr);
          capacity_ = std::exchange(o.capacity_, 0);
          return *this;
          };

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
          cleanup();
          al = std::move(o_al);
          return steal_no_cleanup();
        }
        else {
          if constexpr (allocator_traits::is_always_equal::value)
          {
            cleanup();
            return steal_no_cleanup();
          }
          else {
            if (al == o_al)
            {
              cleanup();
              return steal_no_cleanup();
            }
            else
            {
              assign(o, detail::move_tag{});
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

        auto& data = pair_.second_;
        auto& o_data = o.pair_.second_;

        if constexpr (allocator_traits::propagate_on_container_copy_assignment::value) {
          //change allocator, allocate with copy ctor from new one
          if (al != o_al) {
            cleanup();
            al = o_al;
          }
        }

        assign(o, detail::copy_tag{});
        return *this;
      };

      value_type& operator[](size_t idx) noexcept {
#ifdef DEBUG
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
#endif // DEBUG

        return pair_.second_.first[idx];
      };

      value_type& operator[](size_t idx) const noexcept {
        ASSERT_ABORT(idx < size(), "heap_array index out of bounds");
        return pair_.second_.first[idx];
      };

      NODISCARD CONSTEXPR pointer data() const noexcept {
        return pair_.second_.first;
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
        return static_cast<size_type>(pair_.second_.second - pair_.second_.first);
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

      NODISCARD CONSTEXPR pointer at(size_t index) {
        if (index >= size()) throw std::out_of_range("index out of range");
        return std::addressof(pair_.second_.first[index]);
      }

      NODISCARD CONSTEXPR const pointer at(size_t index) const {
        if (index >= size()) throw std::out_of_range("index out of range");
        return std::addressof(pair_.second_.first[index]);
      }

      NODISCARD CONSTEXPR iterator begin() noexcept {
        //return iterator(pair_.second_.first);
        return ITER_DEBUG_WRAP(iterator, pair_.second_.first);
      };

      NODISCARD CONSTEXPR iterator end() noexcept {
        return ITER_DEBUG_WRAP(iterator, pair_.second_.second);
      };

      NODISCARD CONSTEXPR const_iterator end() const noexcept {
        return const_iterator(this, pair_.second_.second);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.first);
      };

      NODISCARD CONSTEXPR const_iterator cend() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.second);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return ITER_DEBUG_WRAP(const_iterator, pair_.second_.first);
      };

    private:
      CONSTEXPR void cleanup_and_reserve_with_grow() {
        auto& data = pair_.second_;
        auto& first = data.first;
        auto& last = data.second;

        size_type new_capacity = grow_policy::calculate_grow(capacity_);

#ifdef DEBUG
        release_proxy();
#endif // DEBUG
        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
        }
        allocate_buffer(new_capacity);
      };

      NODISCARD CONSTEXPR pointer reallocate(size_type new_capacity) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first;
        auto& last = data.second;

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
#ifdef DEBUG
        release_proxy();
#endif // DEBUG
        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
        }
        
        guard.release();
        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
      };

      CONSTEXPR void resize_buffer(size_type new_capacity)  {
        if (new_capacity == capacity_) return;

        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first;
        auto& last = data.second;

        if (new_capacity > capacity_) {
          reallocate(new_capacity);
          return;
        }
        size_type to_move = std::min(size(), new_capacity);
        pointer new_data = alloc.allocate(new_capacity);
        pointer new_last = new_data;

        internal::realloc_guard<allocator_type> guard(alloc, new_data, new_capacity);
        if (first != nullptr)
        {
          //guard, if exception occurs during move/copy: destroy new_data, leave old intact
          if constexpr (noexcept(internal::uninitialized_move(std::declval<pointer>(), std::declval<pointer>(), std::declval<pointer>(), std::declval<allocator_type&>())))
          {
            new_last = internal::uninitialized_move(first, first + to_move, new_data, alloc);
          }
          else
          {
            new_last = internal::uninitialized_copy(first, first + to_move, new_data, alloc);
          }
        }
#ifdef DEBUG
        release_proxy();
#endif // DEBUG
        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
        }

        guard.release();

        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
        return;
      };

      // tag to select between copy and move
      template<typename Tag>
      CONSTEXPR void clean_and_create_buffer_src(size_type new_capacity, pointer src, size_type new_size, Tag&&) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();
        auto& first = data.first;
        auto& last = data.second;

        pointer new_data = alloc.allocate(new_capacity);
        pointer new_last = new_data;

        //guard, if exception occurs during move/copy: destroy new_data, leave old intact 
        internal::realloc_guard<allocator_type> guard(alloc, new_data, new_capacity);
        if constexpr (std::is_same_v<detail::copy_tag, Tag>) {
          new_last = internal::uninitialized_copy(src, src + new_size, new_data, alloc);
        }
        else
        {
          new_last = internal::uninitialized_move(src, src + new_size, new_data, alloc);
        }
        guard.release();

#ifdef DEBUG
        release_proxy();
#endif // DEBUG
        if (first != nullptr)
        {
          destroy_range(first, last);
          deallocate_buffer();
        }

        first = new_data;
        last = new_last;
        capacity_ = new_capacity;
      };

      CONSTEXPR void cleanup() noexcept {
        auto& data = pair_.second_;
        auto& first = data.first;
        auto& last = data.second;
        if (first != nullptr)
        {
#ifdef DEBUG
          release_proxy();
#endif // DEBUG
          destroy_range(first, last);
          deallocate_buffer();
        }
      };

      CONSTEXPR void allocate_buffer(size_t n) noexcept(noexcept(std::declval<allocator_type&>().allocate(std::declval<size_t>()))) {
        auto& alloc = get_allocator();
        auto& data = pair_.second_;
        pointer new_data = alloc.allocate(n);
        data.first = new_data;
        data.second = new_data;
        capacity_ = n;
      };

      CONSTEXPR void allocate_buffer_non_zero(size_t n) noexcept(noexcept(allocate_buffer(std::declval<size_t>()))) {
        auto& data = pair_.second_;

        if (n <= 0)
        {
          data.first = nullptr;
          data.second = nullptr;
          return;
        }

        allocate_buffer(n);
      };

      CONSTEXPR void deallocate_buffer() noexcept(noexcept(std::declval<allocator_type&>().deallocate(std::declval<pointer>(), std::declval<size_type>()))) {
        auto& alloc = get_allocator();
        auto& data = pair_.second_;
        auto& first = data.first;
        auto& last = data.second;
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

        auto& first = data.first;
        auto& last = data.second;
        pointer end = first + capacity_;
        constexpr uint8_t args_count = sizeof...(Args);
        if (count != 0)
        {
          allocate_buffer(count);
          cleanup_guard guard(this);
          if constexpr (args_count == 0) {
            last = internal::uninitialized_default_construct(first, last, alloc);
          }
          else if constexpr (args_count == 1) {
            last = internal::uninitialized_fill_n(first, capacity_, std::forward<Args>(args)..., alloc);
          }
          else if constexpr (args_count == 2) {
            last = internal::uninitialized_copy(std::forward<Args>(args)..., first, alloc);
          }
          guard.release();
        }
      };

      template<typename Tag>
      CONSTEXPR void assign(const heap_array& o, Tag&& t) {
        auto& data = pair_.second_;
        auto& alloc = get_allocator();

        auto& first = data.first;
        auto& last = data.second;

        auto o_data = o.pair_.second_;
        auto o_first = o_data.first;

        size_type o_cap = o.capacity();
        size_type o_size = o.size();
        size_type size = static_cast<size_type>(last - first); // count of alive objects & default constructed 

        // allocate new buffer, copy/move-construct all
        if (o_size > capacity_) {
          clean_and_create_buffer_src(o_cap, o_first, o_size, t);
          return;
        }

        // copy/move-assign within existing buffer, copy-construct rest
        if (o_size > size) {
          size_type common = size;
          size_type rest = o_size - size;
          if constexpr (std::is_same_v<detail::copy_tag, Tag>) {
            pointer mid = internal::copy_assign_n(o_first, common, first, alloc);
            mid = internal::uninitialized_copy_n(o_first + common, rest, mid, alloc);
            last = mid;
          }
          else {
            pointer mid = internal::move_assign_n(o_first, common, first, alloc);
            mid = internal::uninitialized_move_n(o_first + common, rest, mid, alloc);
            last = mid;
          }
          return;
        }
        // copy/move-assign within existing buffer, destroy rest
        else
        {
          size_type common = o_size;
          size_type rest = size - o_size;
          if constexpr (std::is_same_v<detail::copy_tag, Tag>) {
            pointer mid = internal::copy_assign_n(o_first, common, first, alloc);
            destroy_range(mid, mid + rest);
            last = mid;
          }
          else {
            pointer mid = internal::move_assign_n(o_first, common, first, alloc);
            destroy_range(mid, mid + rest);
            last = mid;
          }
        }
      }

    protected:
      utility::compressed_pair<allocator_type, std::pair<pointer, pointer>> pair_;
      size_type capacity_;
    };
  }
}