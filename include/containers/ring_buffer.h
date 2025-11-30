//#pragma once
//#include <type_traits>
//#include <memory>
//#include <utility.h>
//#include <vector>
//#include <exception>
//#include <chrono>
//#include <xmemory>
//#include <cstddef >
//#include <containers/containers_internal.h>
//
//namespace containers
//{
//  namespace detail {
//    template<typename Traits>
//    class ConstRingBufferIterator {
//      using value_type = typename Traits::value_type;
//      using pointer = typename Traits::pointer;
//      using const_pointer = typename Traits::const_pointer;
//      using reference = typename Traits::const_reference;
//      using difference_type = std::ptrdiff_t;
//      using iterator_category = std::random_access_iterator_tag;
//
//    public:
//      _CONSTEXPR20 ConstRingBufferIterator(pointer ptr, pointer floor, size_t capacity) : ptr_(ptr), floor_(floor), capacity_(capacity) {}
//      _NODISCARD _CONSTEXPR20 reference operator*() const noexcept { return *ptr_; }
//      _NODISCARD _CONSTEXPR20 const_pointer operator->() const noexcept { return ptr_; }
//      _CONSTEXPR20 ConstRingBufferIterator& operator++() noexcept {
//        if (ptr_ == floor_ + capacity_)
//          ptr_ = floor_;
//        else
//          ++ptr_;
//        return *this;
//      }
//      _CONSTEXPR20 ConstRingBufferIterator operator++(int) noexcept {
//        ConstRingBufferIterator tmp = *this;
//        if (ptr_ < floor_ + capacity_)
//          ++(*this);
//        else
//          ptr_ = floor_;
//        return tmp;
//      }
//      _CONSTEXPR20 ConstRingBufferIterator& operator--() noexcept {
//        if (ptr_ > floor_)
//          --ptr_;
//        else
//          ptr_ = floor_ + capacity_;
//        return *this;
//      }
//      _CONSTEXPR20 ConstRingBufferIterator operator--(int) noexcept {
//        ConstRingBufferIterator tmp = *this;
//        if (ptr_ > floor_)
//          --(*this);
//        else
//          ptr_ = floor_ + capacity_;
//        return tmp;
//      }
//      _CONSTEXPR20 ConstRingBufferIterator& operator+=(const difference_type off) noexcept {
//        size_t idx = normalize_index((ptr_ - floor_) + off);
//        ptr_ = floor_ + idx;
//        return *this;
//      }
//
//      _NODISCARD _CONSTEXPR20 ConstRingBufferIterator operator+(const difference_type off) const noexcept {
//        ConstRingBufferIterator tmp = *this;
//        tmp += off;
//        return tmp;
//      }
//
//      _CONSTEXPR20 ConstRingBufferIterator& operator-=(const difference_type off) noexcept {
//        size_t idx = normalize_index((ptr_ - floor_) - off);
//        ptr_ = floor_ + idx;
//        return *this;
//      }
//
//      _NODISCARD _CONSTEXPR20 ConstRingBufferIterator operator-(const difference_type off) const noexcept {
//        ConstRingBufferIterator tmp = *this;
//        tmp -= off;
//        return tmp;
//      }
//
//      _CONSTEXPR20 difference_type operator-(const ConstRingBufferIterator& other) const noexcept {
//        return static_cast<difference_type>(ptr_ - other.ptr_);
//      }
//
//      _CONSTEXPR20 bool operator==(const ConstRingBufferIterator& other) const noexcept {
//        return ptr_ == other.ptr_;
//      }
//      _CONSTEXPR20 bool operator!=(const ConstRingBufferIterator& other) const noexcept {
//        return ptr_ != other.ptr_;
//      }
//      _NODISCARD _CONSTEXPR20 reference operator[](const difference_type _Off) const noexcept {
//        size_t idx = normalize_index((ptr_ - floor_) + _Off);
//        ptr_ = floor_ + idx;
//        return *this;
//      }
//    private:
//      _NODISCARD _CONSTEXPR20 size_t normalize_index(ptrdiff_t index) const noexcept {
//        ptrdiff_t idx = index % static_cast<ptrdiff_t>(capacity_);
//        if (idx < 0) idx += capacity_;
//        return static_cast<size_t>(idx);
//      }
//    protected:
//      pointer ptr_;
//      pointer floor_;
//      size_t capacity_;
//
//    };
//
//    template<typename Traits>
//    class RingBufferIterator : public ConstRingBufferIterator<Traits> {
//      using base_type = ConstRingBufferIterator<Traits>;
//      using pointer = typename Traits::pointer;
//      using reference = typename Traits::reference;
//      using difference_type = std::ptrdiff_t;
//      using iterator_category = std::random_access_iterator_tag;
//    public:
//      _CONSTEXPR20 RingBufferIterator(pointer ptr, pointer floor, size_t capacity) : ConstRingBufferIterator<Traits>(ptr, floor, capacity) {}
//      _NODISCARD _CONSTEXPR20 reference operator*() const noexcept {
//        return const_cast<reference>(base_type::operator*());
//      }
//      _NODISCARD _CONSTEXPR20 pointer operator->() const noexcept {
//        return this->ptr_;
//      }
//      _CONSTEXPR20 RingBufferIterator& operator++() noexcept {
//        base_type::operator++();
//        return *this;
//      }
//      _CONSTEXPR20 RingBufferIterator operator++(int) noexcept {
//        RingBufferIterator tmp = *this;
//        base_type::operator++();
//        return tmp;
//      }
//      _CONSTEXPR20 RingBufferIterator& operator--() noexcept {
//        base_type::operator--();
//        return *this;
//      }
//      _CONSTEXPR20 RingBufferIterator operator--(int) noexcept {
//        RingBufferIterator tmp = *this;
//        base_type::operator--();
//        return tmp;
//      }
//      _CONSTEXPR20 RingBufferIterator& operator+=(const difference_type off) noexcept {
//        base_type::operator+=(off);
//        return *this;
//      }
//      _CONSTEXPR20 RingBufferIterator& operator-=(const difference_type off) noexcept {
//        base_type::operator-=(off);
//        return *this;
//      }
//      _CONSTEXPR20 RingBufferIterator operator+(const difference_type off) const noexcept {
//        RingBufferIterator tmp = *this;
//        tmp += off;
//        return tmp;
//      }
//      _CONSTEXPR20 RingBufferIterator operator-(const difference_type off) const noexcept {
//        RingBufferIterator tmp = *this;
//        tmp -= off;
//        return tmp;
//      }
//      _CONSTEXPR20 difference_type operator-(const RingBufferIterator& other) const noexcept {
//        return base_type::operator-(other);
//      }
//      _CONSTEXPR20 bool operator==(const RingBufferIterator& other) const noexcept {
//        return base_type::operator==(other);
//      }
//      _CONSTEXPR20 bool operator!=(const RingBufferIterator& other) const noexcept {
//        return base_type::operator!=(other);
//      }
//      _NODISCARD _CONSTEXPR20 reference operator[](const difference_type off) const noexcept {
//        return const_cast<reference>(base_type::operator[](off));
//      }
//    };
//
//    template<typename Traits>
//    struct RingBufferVal {
//      using value_type = typename Traits::value_type;
//      using size_type = typename Traits::size_type;
//      using pointer = typename Traits::pointer;
//      using const_pointer = typename Traits::const_pointer;
//      using reference = value_type&;
//      using const_reference = const value_type&;
//
//      _CONSTEXPR20 RingBufferVal() noexcept : first_(nullptr), last_(nullptr), end_(nullptr) {}
//
//      _CONSTEXPR20 RingBufferVal(pointer first, pointer last, pointer end) noexcept
//        : first_(first), last_(last), end_(end) {}
//
//      _CONSTEXPR20 void swap_contents(RingBufferVal& other) noexcept {
//        std::swap(first_, other.first_);
//        std::swap(last_, other.last_);
//        std::swap(end, other.end_);
//      }
//      
//      _CONSTEXPR20 void take_contents(RingBufferVal&& o) noexcept {
//        first_ = o.first_;
//        last_  = o.last_;
//        end_   = o.end_;
//
//        o.first_ = nullptr;
//        o.last_  = nullptr;
//        o.end_   = nullptr;
//      }
//
//      pointer first_;
//      pointer last_;
//      pointer end_;
//    };
//  }
//
//  //TODO: fallback on allocate == nullptr
//  //TODO: proxy(tidy, resise, reserve), dynamic resizing
//  template<typename T, typename Alloc = std::allocator<T>>
//  class ring_buffer
//  {
//    template<typename Traits> friend class internal::tidy_guard;
//    using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
//    using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
//    using tidy_guard = internal::tidy_guard<ring_buffer>;
//  public:
//    using value_type = T;
//    using allocator_type = Alloc;
//    using allocator_traits = typename std::allocator_traits<allocator_type>;
//    using size_type = typename allocator_traits::size_type;
//    using reference = T&;
//    using const_reference = const T&;
//    using pointer = typename allocator_traits::pointer;
//    using const_pointer = typename allocator_traits::const_pointer;
//    using const_iterator = detail::ConstRingBufferIterator<ring_buffer>;
//    using iterator = detail::RingBufferIterator<ring_buffer>;
//    using Val = detail::RingBufferVal<ring_buffer>;
//
//    static_assert(std::is_same_v<typename allocator_traits::value_type, T>, "Allocator::value_type must be same as T");
//    constexpr static bool VTrivial = std::is_trivially_destructible<T>::value && std::is_trivially_copyable<T>::value;
//
//    _CONSTEXPR20 ring_buffer() noexcept
//      : size_(0), capacity_(0), pair_(FirstZeroSecondArgs{}) {
//    };
//
//    _CONSTEXPR20 ring_buffer(const ring_buffer& o) noexcept(noexcept(allocate_buffer(std::declval<size_type>())) && )
//      : size_(o.size_), capacity_(o.capacity_), pair_(FirstOneSecondArgs{}, o.pair_.first()) {
//      allocate_buffer(capacity_);
//      tidy_guard guard(this);
//      internal::construct_range(pair_.first(), pair_.second().first_, pair_.second().end_, o.pair_.second().first_, o.pair_.second().end_);
//      guard.release();
//    };
//
//    _CONSTEXPR20 ring_buffer(ring_buffer&& o) noexcept()
//      : capacity_(o.capacity_), size_(o.size_), pair_(FirstOneSecondArgs{}, std::move(o.pair_.first())) {
//      s
//    }
//
//    _CONSTEXPR20 ring_buffer(size_type n, allocator_type& a = Alloc())
//      noexcept()
//      : capacity_(n), size_(0), pair_(FirstOneSecondArgs{}, a)
//    {
//      allocate_buffer(n);
//      tidy_guard guard(this);
//      internal::construct_range(pair_.first(), pair_.second().first_, pair_.second().end_);
//      guard.release();
//    };
//
//    _CONSTEXPR20 ~ring_buffer() noexcept(noexcept(tidy())) {
//      tidy();
//    };
//
//    ring_buffer& operator=(const ring_buffer& o)
//      noexcept()
//    {
//      auto& data = pair_.second();
//
//      auto& first = data.first_;
//      auto& last  = data.last_;
//      auto& end   = data.end_;
//
//      constexpr bool pocca = allocator_traits::propagate_on_container_copy_assignment::value;
//      constexpr bool always_equal = allocator_traits::is_always_equal::value;
//      if constexpr (pocca && !always_equal) {
//        if (!(pair_.first() == o.first()))
//        {
//          tidy();
//          pair_.first() = o.first();
//        }
//      }
//      allocate_buffer(o.capacity_);
//      tidy_guard guard(this);
//      internal::construct_range(pair_.first(), first, end, );
//      guard.release();
//    }
//
//    ring_buffer& operator=(ring_buffer&& o)
//      noexcept()
//    {
//
//    }
//
//    _NODISCARD _CONSTEXPR20 size_type size() const noexcept {
//      return size_;
//    }
//    _NODISCARD _CONSTEXPR20 allocator_type& get_allocator() noexcept {
//      return pair_.first();
//    }
//    _NODISCARD _CONSTEXPR20 pointer data() noexcept {
//      return pair_.second().first_;
//    }
//    _NODISCARD _CONSTEXPR20 void get_interval(size_type start, size_type count, ring_buffer& out) const
//      noexcept(
//        noexcept(out.reserve(std::declval<size_type>())) &&
//        std::is_nothrow_copy_assignable_v<T> || VTrivial
//        )
//    {
//      if (count > capacity_) {
//        std::cerr << "Requested count exceeds ring_buffer size";
//        return;
//      }
//      size_type norm_start = normalize_index(static_cast<int64_t>(start));
//      size_type end_idx = normalize_index(static_cast<int64_t>(start + count));
//      if (out.size() < count) {
//        out.reserve(count);
//      }
//      pointer src_data_start = pair_.second().first_;
//      pointer out_data_start = out.pair_.second().first_;
//      if constexpr (VTrivial) {
//        if (norm_start < end_idx) {
//          std::memcpy(out_data_start, src_data_start + norm_start, sizeof(value_type) * count);
//        }
//        else {
//          size_type first_part = capacity_ - norm_start;
//          std::memcpy(out_data_start, src_data_start + norm_start, sizeof(value_type) * first_part);
//          std::memcpy(out_data_start + first_part, src_data_start, sizeof(value_type) * (count - first_part));
//        }
//      }
//      else {
//        for (size_type i = 0; i < count; i++) {
//          size_type idx = normalize_index(static_cast<int64_t>(start + i));
//          out.pair_.second()[i] = pair_.second()[idx];
//        }
//      }
//    }
//    //TODO: resizing cause UB with iterators, create proxies or other behavior
//    _CONSTEXPR20 void resize(size_type new_size) {
//      auto& alloc = get_allocator();
//      auto& data = pair_.second();
//
//      pointer first = data.first_;
//      pointer last = data.last_;
//      pointer end = data.end_;
//
//      if (first != nullptr)
//      {
//        utility::destroy_range(alloc, first, last);
//        alloc.deallocate(first, first - last);
//      }
//      if (new_size == 0)
//        return;
//
//    }
//
//    _NODISCARD _CONSTEXPR20 size_type capacity() const noexcept {
//      return capacity_;
//    }
//
//    _NODISCARD _CONSTEXPR20 size_type capacity()  noexcept {
//      return capacity_;
//    }
//
//    _NODISCARD _CONSTEXPR20 const_pointer at(size_type index) const noexcept {
//      size_type idx = normalize_index(static_cast<int64_t>(index));
//      return pair_.second().first_ + idx;
//    }
//
//    _CONSTEXPR20 void push(const T& value) noexcept() {
//
//    }
//
//    _CONSTEXPR20 void push(T&& value) noexcept() {
//    }
//
//    _CONSTEXPR20 void clear() noexcept {
//
//    }
//
//    _CONSTEXPR20 void reserve(size_type n)
//      noexcept(
//        noexcept(VTrivial ||)
//        )
//    {
//
//      tidy_guard guard(this);
//      internal::construct_range(pair_.first(), pair_.second().first_, pair_.second().end_);
//      guard.release();
//    }
//
//    _NODISCARD _CONSTEXPR20 iterator begin() noexcept {
//      return iterator(pair_.second().first_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 const_iterator begin() const noexcept {
//      return const_iterator(pair_.second().first_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 iterator head() noexcept {
//      return iterator(pair_.second().last_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 const_iterator head() const noexcept {
//      return const_iterator(pair_.second().last_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 iterator end() noexcept {
//      return iterator(pair_.second().end_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 const_iterator end() const noexcept {
//      return const_iterator(pair_.second().end_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 const_iterator chead() const noexcept {
//      return const_iterator(pair_.second().last_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 const_iterator cend() const noexcept {
//      return const_iterator(pair_.second().end_, pair_.second().first_, capacity_);
//    }
//    _NODISCARD _CONSTEXPR20 reference operator[](size_type index) noexcept {
//      size_type idx = normalize_index(static_cast<int64_t>(index));
//      return pair_.second().first_[idx];
//    }
//    _NODISCARD _CONSTEXPR20 const_reference operator[](size_type index) const noexcept {
//      size_type idx = normalize_index(static_cast<int64_t>(index));
//      return pair_.second().first_[idx];
//    }
//
//  private:
//    _NODISCARD _CONSTEXPR20 size_type normalize_index(ptrdiff_t index) const noexcept {
//      if (index == 0) return 0;
//      ptrdiff_t idx = index % static_cast<ptrdiff_t>(capacity_);
//      if (idx < 0) idx += capacity_;
//      return static_cast<size_type>(idx);
//    }
//
//    _CONSTEXPR20 void tidy() noexcept(std::is_nothrow_destructible_v<value_type>) {
//      if (pair_.second().first_ != nullptr) {
//        utility::destroy_range(pair_.first(), pair_.second().first_, pair_.second().last_);
//        deallocate_buffer(pair_.second().first_, capacity_);
//        pair_.second().first_ = nullptr;
//        pair_.second().last_  = nullptr;
//        pair_.second().end_   = nullptr;
//        size_ = 0;
//        capacity_ = 0;
//      }
//    };
//
//    _CONSTEXPR20 void allocate_buffer(size_type n) {
//      auto& data = pair_.second();
//      auto& alloc = pair_.first();
//      auto& first = data.first_;
//      auto& last  = data.last_;
//      auto& end   = data.end_;
//
//      if (n <= 0) {
//        first = nullptr;
//        last  = nullptr;
//        end   = nullptr;
//        return;
//      }
//
//      pointer new_data = allocator_traits::allocate(pair_.first(), n);
//      first = new_data;
//      last  = new_data;
//      end   = new_data + n;
//    }
//    
//    _CONSTEXPR20 void deallocate_buffer(pointer p, size_type n) noexcept {
//      allocator_traits::deallocate(pair_.first(), p, n);
//    }
//
//  private:
//    utility::compressed_pair<Alloc, Val> pair_;
//    size_type size_; // alive objects
//    size_type head_; // current head
//    size_type capacity_;
//  };
//}