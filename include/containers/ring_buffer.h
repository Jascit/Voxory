#pragma once
#include <type_traits>
#include <memory>
#include <utility.h>
#include <vector>
#include <exception>

namespace containers
{
  namespace detail {
    template<typename Traits>
    class ConstRingBufferIterator {
      using value_type = typename Traits::value_type;
      using pointer = typename Traits::pointer;
      using const_pointer = typename Traits::const_pointer;
      using reference = typename Traits::const_reference;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::random_access_iterator_tag;

    public:
      _CONSTEXPR20 ConstRingBufferIterator(pointer ptr, pointer floor, size_t capacity) : ptr_(ptr), floor_(floor), capacity_(capacity) {}
      _NODISCARD _CONSTEXPR20 reference operator*() const noexcept { return *ptr_; }
      _NODISCARD _CONSTEXPR20 const_pointer operator->() const noexcept { return ptr_; }
      _CONSTEXPR20 ConstRingBufferIterator& operator++() noexcept {
        if (ptr_ == floor_ + capacity_)
          ptr_ = floor_;
        else
          ++ptr_;
        return *this;
      }
      _CONSTEXPR20 ConstRingBufferIterator operator++(int) noexcept {
        ConstRingBufferIterator tmp = *this;
        if (ptr_ < floor_ + capacity_)
          ++(*this);
        else
          ptr_ = floor_;
        return tmp;
      }
      _CONSTEXPR20 ConstRingBufferIterator& operator--() noexcept {
        if (ptr_ > floor_)
          --ptr_;
        else
          ptr_ = floor_ + capacity_;
        return *this;
      }
      _CONSTEXPR20 ConstRingBufferIterator operator--(int) noexcept {
        ConstRingBufferIterator tmp = *this;
        if (ptr_ > floor_)
          --(*this);
        else
          ptr_ = floor_ + capacity_;
        return tmp;
      }
      _CONSTEXPR20 ConstRingBufferIterator& operator+=(const difference_type off) noexcept {
        size_t idx = normalize_index((ptr_ - floor_) + off);
        ptr_ = floor_ + idx;
        return *this;
      }

      _NODISCARD _CONSTEXPR20 ConstRingBufferIterator operator+(const difference_type off) const noexcept {
        ConstRingBufferIterator tmp = *this;
        tmp += off;
        return tmp;
      }

      _CONSTEXPR20 ConstRingBufferIterator& operator-=(const difference_type off) noexcept {
        size_t idx = normalize_index((ptr_ - floor_) - off);
        ptr_ = floor_ + idx;
        return *this;
      }

      _NODISCARD _CONSTEXPR20 ConstRingBufferIterator operator-(const difference_type off) const noexcept {
        ConstRingBufferIterator tmp = *this;
        tmp -= off;
        return tmp;
      }

      _CONSTEXPR20 difference_type operator-(const ConstRingBufferIterator& other) const noexcept {
        return static_cast<difference_type>(ptr_ - other.ptr_);
      }

      _CONSTEXPR20 bool operator==(const ConstRingBufferIterator& other) const noexcept {
        return ptr_ == other.ptr_;
      }
      _CONSTEXPR20 bool operator!=(const ConstRingBufferIterator& other) const noexcept {
        return ptr_ != other.ptr_;
      }
      _NODISCARD _CONSTEXPR20 reference operator[](const difference_type _Off) const noexcept {
        size_t idx = normalize_index((ptr_ - floor_) + _Off);
        return *(*this + idx);
      }
    private:
      _NODISCARD _CONSTEXPR20 size_t normalize_index(int64_t index) const noexcept {
        int64_t m = static_cast<int64_t>(capacity_);
        int64_t idx = (index % m + m) % m;
        return idx;
      }
    protected:
      pointer ptr_;
      pointer floor_;
      size_t capacity_;

    };

    template<typename Traits>
    class RingBufferIterator : public ConstRingBufferIterator<Traits> {
      using base_type = ConstRingBufferIterator<Traits>;
      using pointer = typename Traits::pointer;
      using reference = typename Traits::reference;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::random_access_iterator_tag;
    public:
      _CONSTEXPR20 RingBufferIterator(pointer ptr, pointer floor, size_t capacity) : ConstRingBufferIterator<Traits>(ptr, floor, capacity) {}
      _NODISCARD _CONSTEXPR20 reference operator*() const noexcept {
        return const_cast<reference>(base_type::operator*());
      }
      _NODISCARD _CONSTEXPR20 pointer operator->() const noexcept {
        return this->ptr_;
      }
      _CONSTEXPR20 RingBufferIterator& operator++() noexcept {
        base_type::operator++();
        return *this;
      }
      _CONSTEXPR20 RingBufferIterator operator++(int) noexcept {
        RingBufferIterator tmp = *this;
        base_type::operator++();
        return tmp;
      }
      _CONSTEXPR20 RingBufferIterator& operator--() noexcept {
        base_type::operator--();
        return *this;
      }
      _CONSTEXPR20 RingBufferIterator operator--(int) noexcept {
        RingBufferIterator tmp = *this;
        base_type::operator--();
        return tmp;
      }
      _CONSTEXPR20 RingBufferIterator& operator+=(const difference_type off) noexcept {
        base_type::operator+=(off);
        return *this;
      }
      _CONSTEXPR20 RingBufferIterator& operator-=(const difference_type off) noexcept {
        base_type::operator-=(off);
        return *this;
      }
      _CONSTEXPR20 RingBufferIterator operator+(const difference_type off) const noexcept {
        RingBufferIterator tmp = *this;
        tmp += off;
        return tmp;
      }
      _CONSTEXPR20 RingBufferIterator operator-(const difference_type off) const noexcept {
        RingBufferIterator tmp = *this;
        tmp -= off;
        return tmp;
      }
      _CONSTEXPR20 difference_type operator-(const RingBufferIterator& other) const noexcept {
        return base_type::operator-(other);
      }
      _CONSTEXPR20 bool operator==(const RingBufferIterator& other) const noexcept {
        return base_type::operator==(other);
      }
      _CONSTEXPR20 bool operator!=(const RingBufferIterator& other) const noexcept {
        return base_type::operator!=(other);
      }
      _NODISCARD _CONSTEXPR20 reference operator[](const difference_type off) const noexcept {
        return const_cast<reference>(base_type::operator[](off));
      }
    };
  }


  template<typename T, typename Alloc = std::allocator<T>>
  class RingBuffer
  {
    friend class RingBuffer;
    using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
    using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
  public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using allocator_traits = typename std::allocator_traits<allocator_type>;
    using reference = T&; 
    using const_reference = const T&;
    using pointer = typename allocator_traits::pointer;
    using const_pointer = allocator_traits::const_pointer;
    using const_iterator = detail::ConstRingBufferIterator<RingBuffer>;
    using iterator = detail::RingBufferIterator<RingBuffer>;

    constexpr static bool VTrivial = std::is_trivially_destructible<T>::value && std::is_trivially_copyable<T>::value;

    _CONSTEXPR20 RingBuffer(RingBuffer& o) 
      noexcept(
        std::is_nothrow_copy_constructible_v<allocator_type> &&
        noexcept(allocator_traits::allocate(std::declval<allocator_type&>(), std::declval<size_type>())) &&
        noexcept(utility::uninitialized_copy_n(std::declval<allocator_type&>(), std::declval<pointer>(), std::declval<size_type>(), std::declval<pointer>()))
      )
      : pair_(FirstOneSecondArgs{}, std::forward<allocator_type&>(o.get_allocator())), size_(o.size_), head_(o.head_)
    {
      pair_.second() = allocator_traits::allocate(pair_.first(), o.size_);
      utility::uninitialized_copy_n(pair_.first(), o.pair_.second(), o.size_, pair_.second());
    };

    _CONSTEXPR20 RingBuffer(RingBuffer&& o) noexcept(
      noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
      )
      : pair_(FirstOneSecondArgs{}, std::move(o.get_allocator()), std::move(o.pair_.second())), head_(o.head_), size_(o.size_) {
      o.head_ = 0;
      o.size_ = 0;
      o.pair_.second() = nullptr;
    }

    _CONSTEXPR20 RingBuffer(size_type n)
      noexcept(
        noexcept(utility::uninitialized_default_construct_n(std::declval<allocator_type&>(), std::declval<pointer>(), std::declval<size_type>()))
        )
      : size_(n), pair_(FirstZeroSecondArgs{}), head_(0)
    {
      pair_.second() = pair_.first().allocate(n);
      utility::uninitialized_default_construct_n(pair_.first(), pair_.second(), n);
    }

    _CONSTEXPR20 RingBuffer(size_type n, allocator_type& a)
      noexcept(
          noexcept(utility::uninitialized_default_construct_n(std::declval<allocator_type&>(), std::declval<pointer>(), std::declval<size_type>()))
        )
      : size_(n), pair_(FirstOneSecondArgs{}, a), head_(0)
    {
      pair_.second() = allocator_traits::allocate(pair_.first(), n);
      utility::uninitialized_default_construct_n(pair_.first(), pair_.second(), n);
    };

    _CONSTEXPR20 ~RingBuffer() noexcept(VTrivial || std::is_nothrow_destructible_v<value_type>){
      if (pair_.second() == nullptr)
      {
        return;
      }
      if constexpr (!VTrivial)
      {
        for (size_t i = 0; i < size_; i++)
        {
          allocator_traits::destroy(pair_.first(), pair_.second() + i);
        }
      }
      allocator_traits::deallocate(pair_.first(), pair_.second(), size_);
      pair_.second() = nullptr;
    };
    _NODISCARD _CONSTEXPR20 size_type size() const noexcept {
      return size_;
    }
    _NODISCARD _CONSTEXPR20 allocator_type& get_allocator() noexcept {
      return pair_.first();
    }
    _NODISCARD _CONSTEXPR20 const allocator_type& get_allocator() const noexcept {
      return pair_.first();
    }
    _NODISCARD _CONSTEXPR20 pointer data() noexcept {
      return pair_.second();
    }

    _CONSTEXPR20 void push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
      pair_.second()[head_] = value;
      if (head_ < size_-1)
        head_++;
      else
        head_ = 0;
    }

    _NODISCARD _CONSTEXPR20 iterator head() noexcept {
      return iterator(pair_.second() + head_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 const_iterator head() const noexcept {
      return const_iterator(pair_.second() + head_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 iterator end() noexcept {
      return iterator(pair_.second() + size_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 const_iterator end() const noexcept {
      return const_iterator(pair_.second() + size_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 const_iterator chead() const noexcept {
      return const_iterator(pair_.second() + head_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 const_iterator cend() const noexcept {
      return const_iterator(pair_.second() + size_, pair_.second(), size_);
    }
    _NODISCARD _CONSTEXPR20 reference operator[](size_type index) noexcept {
      size_type idx = normalize_index(static_cast<int64_t>(index));
      return pair_.second()[idx];
    }
    _NODISCARD _CONSTEXPR20 const_reference operator[](size_type index) const noexcept {
      size_type idx = normalize_index(static_cast<int64_t>(index));
      return pair_.second()[idx];
    }

    RingBuffer& operator=(const RingBuffer& o)
      noexcept(
        std::is_nothrow_copy_assignable_v<allocator_type> &&
        noexcept(utility::initialized_copy_n(std::declval<allocator_type&>(), std::declval<pointer>(), std::declval<size_type>(), std::declval<pointer>()))
      )
    {
      if (this != &o) {
        if (pair_.second() != nullptr) {
          if constexpr (!VTrivial) {
            for (size_t i = 0; i < size_; i++) {
              allocator_traits::destroy(pair_.first(), pair_.second() + i);
            }
          }
          allocator_traits::deallocate(pair_.first(), pair_.second(), size_);
        }
        pair_.copy(o.pair_);
        size_ = o.size_;
        head_ = o.head_;
        pair_.second() = allocator_traits::allocate(pair_.first(), o.size_);
        utility::initialized_copy_n(pair_.first(), o.pair_.second(), o.size_, pair_.second());
      }
      return *this;
    }

    RingBuffer& operator=(RingBuffer&& o) 
      noexcept(
        std::is_nothrow_move_assignable_v<allocator_type>
      )
    {
      if (this != &o) {
        if (pair_.second() != nullptr) {
          if constexpr (!VTrivial) {
            for (size_t i = 0; i < size_; i++) {
              allocator_traits::destroy(pair_.first(), pair_.second() + i);
            }
          }
          allocator_traits::deallocate(pair_.first(), pair_.second(), size_);
        }
        pair_.copy(o.pair_);
        size_ = o.size_;
        head_ = o.head_;
        o.head_ = 0;
        o.size_ = 0;
        o.pair_.second() = nullptr;
      }
      return *this;
    }

  private:
    _NODISCARD _CONSTEXPR20 size_type normalize_index(int64_t index) const noexcept {
      int64_t m = static_cast<int64_t>(size_);
      int64_t idx = (index % size_ + size_) % size_;
      return idx;
    }
  private:
    utility::compressed_pair<Alloc, pointer> pair_;
    size_type head_;
    size_type size_;
  };
}