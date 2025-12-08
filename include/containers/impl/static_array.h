#pragma once
#define NOMINMAX
#include <utility.h>
#include <type_traits.h>
#include <containers/detail/containers_internal.h>
#include <platform/platform.h>
#include <memory>
#include <xmemory>
namespace voxory {
  namespace containers {
    namespace detail {
      // ConstStaticArrayIterator + StaticArrayIterator
// Зручні, читабельні ітератори для статичного масиву (const + non-const)
// Зберігаю твої макроси: CONSTEXPR, NODISCARD

      template<typename Traits>
      class ConstStaticArrayIterator {
      public:
        // types
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::pointer;
        using const_pointer = typename Traits::const_pointer;
        using reference = typename Traits::const_reference; // const iterator -> const_reference
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        // ctors
        CONSTEXPR ConstStaticArrayIterator() noexcept : ptr_(nullptr) {}
        CONSTEXPR explicit ConstStaticArrayIterator(pointer p) noexcept : ptr_(p) {}

        // dereference
        NODISCARD CONSTEXPR reference operator*() const noexcept { return *ptr_; }
        NODISCARD CONSTEXPR const_pointer operator->() const noexcept { return ptr_; }

        // prefix / postfix ++
        CONSTEXPR ConstStaticArrayIterator& operator++() noexcept {
          ++ptr_;
          return *this;
        }
        CONSTEXPR ConstStaticArrayIterator operator++(int) noexcept {
          ConstStaticArrayIterator tmp = *this;
          ++(*this); // reuse prefix
          return tmp;
        }

        // prefix / postfix --
        CONSTEXPR ConstStaticArrayIterator& operator--() noexcept {
          --ptr_;
          return *this;
        }
        CONSTEXPR ConstStaticArrayIterator operator--(int) noexcept {
          ConstStaticArrayIterator tmp = *this;
          --(*this);
          return tmp;
        }

        // random-access arithmetic
        CONSTEXPR ConstStaticArrayIterator& operator+=(difference_type off) noexcept {
          ptr_ += off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstStaticArrayIterator operator+(difference_type off) const noexcept {
          ConstStaticArrayIterator tmp = *this;
          tmp += off;
          return tmp;
        }

        CONSTEXPR ConstStaticArrayIterator& operator-=(difference_type off) noexcept {
          ptr_ -= off;
          return *this;
        }
        NODISCARD CONSTEXPR ConstStaticArrayIterator operator-(difference_type off) const noexcept {
          ConstStaticArrayIterator tmp = *this;
          tmp -= off;
          return tmp;
        }

        // difference between iterators
        CONSTEXPR difference_type operator-(const ConstStaticArrayIterator& other) const noexcept {
          return static_cast<difference_type>(ptr_ - other.ptr_);
        }

        // comparisons
        CONSTEXPR bool operator==(const ConstStaticArrayIterator& other) const noexcept { return ptr_ == other.ptr_; }
        CONSTEXPR bool operator!=(const ConstStaticArrayIterator& other) const noexcept { return ptr_ != other.ptr_; }
        CONSTEXPR bool operator<(const ConstStaticArrayIterator& other) const noexcept { return ptr_ < other.ptr_; }
        CONSTEXPR bool operator>(const ConstStaticArrayIterator& other) const noexcept { return ptr_ > other.ptr_; }
        CONSTEXPR bool operator<=(const ConstStaticArrayIterator& other) const noexcept { return ptr_ <= other.ptr_; }
        CONSTEXPR bool operator>=(const ConstStaticArrayIterator& other) const noexcept { return ptr_ >= other.ptr_; }

        // random-access indexing
        NODISCARD CONSTEXPR reference operator[](difference_type off) const noexcept {
          return *(ptr_ + off);
        }

      protected:
        pointer ptr_;
      };


      // Non-const iterator: успадковує const-версію і відкриває мутуючі операції
      template<typename Traits>
      class StaticArrayIterator : public ConstStaticArrayIterator<Traits> {
        using base_type = ConstStaticArrayIterator<Traits>;
      public:
        using value_type = typename Traits::value_type;
        using pointer = typename Traits::pointer;
        using reference = typename Traits::reference;
        using difference_type = typename base_type::difference_type;
        using iterator_category = std::random_access_iterator_tag;

        // ctors
        CONSTEXPR StaticArrayIterator() noexcept : base_type(nullptr) {}
        CONSTEXPR explicit StaticArrayIterator(pointer p) noexcept : base_type(p) {}

        // mutable deref (casts away const from base implementation)
        NODISCARD CONSTEXPR reference operator*() const noexcept {
          return const_cast<reference>(base_type::operator*());
        }
        NODISCARD CONSTEXPR pointer operator->() const noexcept {
          return this->ptr_;
        }

        // reuse base implementations for ++/-- and arithmetic via using / delegation
        CONSTEXPR StaticArrayIterator& operator++() noexcept {
          base_type::operator++();
          return *this;
        }
        CONSTEXPR StaticArrayIterator operator++(int) noexcept {
          StaticArrayIterator tmp = *this;
          base_type::operator++();
          return tmp;
        }

        CONSTEXPR StaticArrayIterator& operator--() noexcept {
          base_type::operator--();
          return *this;
        }
        CONSTEXPR StaticArrayIterator operator--(int) noexcept {
          StaticArrayIterator tmp = *this;
          base_type::operator--();
          return tmp;
        }

        CONSTEXPR StaticArrayIterator& operator+=(difference_type off) noexcept {
          base_type::operator+=(off);
          return *this;
        }
        CONSTEXPR StaticArrayIterator& operator-=(difference_type off) noexcept {
          base_type::operator-=(off);
          return *this;
        }

        NODISCARD CONSTEXPR StaticArrayIterator operator+(difference_type off) const noexcept {
          StaticArrayIterator tmp = *this;
          tmp += off;
          return tmp;
        }
        NODISCARD CONSTEXPR StaticArrayIterator operator-(difference_type off) const noexcept {
          StaticArrayIterator tmp = *this;
          tmp -= off;
          return tmp;
        }

        CONSTEXPR difference_type operator-(const StaticArrayIterator& other) const noexcept {
          return base_type::operator-(other);
        }

        CONSTEXPR bool operator==(const StaticArrayIterator& other) const noexcept { return base_type::operator==(other); }
        CONSTEXPR bool operator!=(const StaticArrayIterator& other) const noexcept { return base_type::operator!=(other); }

        NODISCARD CONSTEXPR reference operator[](difference_type off) const noexcept {
          return const_cast<reference>(base_type::operator[](off));
        }
      };



      template<typename Pointer, typename... Args>
      concept IsNoexceptConstructibleRange =
        (sizeof...(Args) == 0 && std::is_nothrow_default_constructible_v<typename std::pointer_traits<Pointer>::element_type>)
        ||
        (sizeof...(Args) == 1 &&
          (std::is_nothrow_copy_constructible_v<typename std::pointer_traits<Pointer>::element_type>
            || type_traits::use_memcpy_copy_construct_v<Pointer>))
        ||
        (sizeof...(Args) == 2 &&
          (std::is_nothrow_copy_constructible_v<typename std::pointer_traits<Pointer>::element_type>
            || type_traits::use_memcpy_copy_construct_v<Pointer>)
          && std::is_nothrow_default_constructible_v<typename std::pointer_traits<Pointer>::element_type>);

      template<typename Pointer, typename... Args>
      inline constexpr bool IsNoexceptConstructibleRange_v = IsNoexceptConstructibleRange<Pointer, Args...>;

      template<typename T>
      class static_array_base {
      public:
        CONSTEXPR virtual size_t capacity() const noexcept = 0;
        CONSTEXPR virtual T* data() const noexcept = 0;
      };

      //alias
      template<typename T>
      struct static_array_traits {
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = size_t;
        using reference = T&;
        using const_reference = const reference;
        using difference_type = std::ptrdiff_t;
      };
    }

    template<typename T, size_t N>
    class static_array : detail::static_array_base<T> {
    public:
      using value_type = T;
      using pointer = T*;
      using size_type = size_t;
      using reference = T&;
      using const_reference = const reference;
      using trivial_traits = type_traits::trivial_traits<value_type>;

      static constexpr size_type capacity_ = N;
      using array_type = std::conditional_t<N == 0, internal::empty_data, unsigned char[N * sizeof(value_type)]>;
      using iterator = detail::StaticArrayIterator<detail::static_array_traits<value_type>>;
      using const_iterator = detail::ConstStaticArrayIterator<detail::static_array_traits<value_type>>;

      ~static_array() {
        cleanup();
      };

      static_array() noexcept(std::is_nothrow_default_constructible_v<value_type>) {
        construct_range(N);
      };

      static_array(const detail::static_array_base<value_type>& o) noexcept(noexcept(construct_range(std::declval<size_type>(), std::declval<pointer>(), std::declval<pointer>()))) {
        size_type o_capacity = o.capacity();
        construct_range(o.capacity(), o.data(), o.data() + o.capacity());
      };

      static_array(detail::static_array_base<value_type>&& o) noexcept(std::is_nothrow_move_constructible_v<value_type>&& std::is_nothrow_default_constructible_v<value_type>) {
        auto& first_ref = pair_.first;
        auto& last_ref = pair_.second;
        first_ref = std::launder(reinterpret_cast<pointer>(&data_));
        size_type n = std::min(capacity_, o.capacity());
        last_ref = std::uninitialized_move(o.data(), o.data() + n, data());
        if (n < capacity_)
          last_ref = std::uninitialized_default_construct_n(first_ref + n, capacity_ - n);
      };

      NODISCARD CONSTEXPR iterator begin() noexcept {
        return iterator(pair_.first);
      };

      NODISCARD CONSTEXPR const_iterator begin() const noexcept {
        return const_iterator(pair_.first);
      };

      NODISCARD CONSTEXPR iterator end() noexcept {
        return iterator(pair_.first + capacity_);
      };

      NODISCARD CONSTEXPR const_iterator end() const noexcept {
        return const_iterator(pair_.first + capacity_);
      };

      NODISCARD CONSTEXPR const_iterator cend() const noexcept {
        return const_iterator(pair_.first + capacity_);
      };

      NODISCARD CONSTEXPR const_iterator cbegin() const noexcept {
        return const_iterator(pair_.first);
      };

      CONSTEXPR static_array& operator=(const detail::static_array_base<value_type>& o) {
        if (this == &o) return *this;
        if (o.capacity() > capacity_) {
          pair_.second = copy_assign(o.data(), o.data() + capacity_, pair_.first);
        }
        else {
          pointer new_last = copy_assign(o.data(), o.data() + capacity_, pair_.first);
          destroy_range(new_last, pair_.second);
          pair_.second = new_last;
          pair_.second = std::uninitialized_default_construct_n(pair_.second, capacity_ - (pair_.second - pair_.first));
        }
        return *this;
      };

      CONSTEXPR static_array& operator=(detail::static_array_base<value_type>&& o) {
        if (this == &o) return *this;
        if (o.capacity() > capacity_) {
          pair_.second = move_assign(o.data(), o.data() + capacity_, pair_.first);
        }
        else {
          pointer new_last = move_assign(o.data(), o.data() + capacity_, pair_.first);
          destroy_range(new_last, pair_.second);
          pair_.second = new_last;
          pair_.second = std::uninitialized_default_construct_n(pair_.second, capacity_ - (pair_.second - pair_.first));
        }
        return *this;
      };

      NODISCARD CONSTEXPR reference operator[](size_type idx) noexcept {
        ASSERT_ABORT(idx < capacity_, "static_array: out of range");
        return data()[idx];
      };

      NODISCARD CONSTEXPR const_reference operator[](size_type idx) const noexcept {
        ASSERT_ABORT(idx >= capacity_, "static_array: out of range");
        return data()[idx];
      };

      NODISCARD CONSTEXPR size_type capacity() const noexcept override {
        return capacity_;
      };

      NODISCARD CONSTEXPR pointer data() const noexcept override {
        return pair_.first;
      };

    private:
      template<typename... Args>
      CONSTEXPR void construct_range(size_t count, Args&&... args) noexcept(detail::IsNoexceptConstructibleRange_v<pointer, Args...>) {
        // Args:
        //  - 0 args: default-construct first `count` elements
        //  - 1 arg: fill first `count` elements with value
        //  - 2 args: copy from [src_begin, src_end) into first `count` elements

        auto& first_ref = pair_.first;
        auto& last_ref = pair_.second;

        // pointer to storage start
        pointer first = std::launder(reinterpret_cast<pointer>(&data_));
        first_ref = first;

        const size_type cap = capacity_;
        const size_type n = std::min(count, cap);

        if constexpr (sizeof...(Args) == 0) {
          // default-construct n elements
          last_ref = std::uninitialized_default_construct_n(first, n);
        }
        else if constexpr (sizeof...(Args) == 1) {
          // fill with value (args... -> value)
          last_ref = std::uninitialized_fill_n(first, n, std::forward<Args>(args)...);
        }
        else {
          last_ref = std::uninitialized_copy_n(type_traits::first_arg(std::forward<Args>(args)...), n, first);

          // If count < capacity, default-construct remaining elements and update last_ref
          if (n < cap) {
            last_ref = std::uninitialized_default_construct_n(first + n, cap - n);
          }
        }
      }
      CONSTEXPR pointer move_assign(pointer first, pointer last, pointer dest) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        if (type_traits::use_memmove_copy_construct_v<pointer>)
        {
          std::size_t count = static_cast<std::size_t>(last - first);
          std::size_t bytes = count * sizeof(value_type);
          std::memmove(dest, first, bytes);
          return dest + count;
        }
        else {
          std::size_t count = static_cast<std::size_t>(last - first);
          pointer new_last = dest + count;
          for (; dest != new_last; ++first) {
            *dest = std::move(*first);
            ++dest;
          }
          return dest;
        }
      }

      CONSTEXPR pointer copy_assign(pointer first, pointer last, pointer dest) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        if (type_traits::use_memmove_copy_construct_v<pointer>)
        {
          std::size_t count = static_cast<std::size_t>(last - first);
          std::size_t bytes = count * sizeof(value_type);
          std::memmove(dest, first, bytes);
          return dest + count;
        }
        else {
          std::size_t count = static_cast<std::size_t>(last - first);
          pointer new_last = dest + count;
          for (; dest != new_last; ++first) {
            *dest = *first;
            ++dest;
          }
          return dest;
        }
      }
      CONSTEXPR void destroy_range(pointer first, pointer end) noexcept(trivial_traits::is_trivially_destructible::value || std::is_nothrow_destructible_v<value_type>) {
        if constexpr (!trivial_traits::is_trivially_destructible::value)
        {
          std::destroy_n(first, static_cast<size_t>(end - first));
        }
      };

      CONSTEXPR void cleanup() noexcept(noexcept(destroy_range(std::declval<pointer>(), std::declval<pointer>()))) {
        auto& first = pair_.first;
        auto& last = pair_.second;
        destroy_range(first, last);
        last = first;
      }
    private:
      alignas(value_type) array_type data_;
      std::pair<pointer, pointer> pair_;
    };
  }
}