#pragma once
#include <type_traits>
#include <xmemory>

namespace utility {
  namespace detail {
    struct FirstOneSecondArgs {
      FirstOneSecondArgs() = default;
    };
    struct FirstZeroSecondArgs {
      FirstZeroSecondArgs() = default;
    };
  }
  template<typename T>
  constexpr void swap(T& first, T& second)
    noexcept(std::is_nothrow_move_constructible_v<T>&& std::is_nothrow_move_assignable_v<T>) {
    T temp_ = std::move(first);
    first = std::move(second);
    second = std::move(temp_);
  }

  template<typename T1, typename T2, bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
  class compressed_pair final {
    T1 first_;
    T2 second_;

  public:
    constexpr void swap(compressed_pair<T1, T2, false>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      T2 temp1_ = std::move(first_);
      T2 temp2_ = std::move(second_);
      second_ = std::move(other.second());
      first_ = std::move(other.first());
      other.second_ = std::move(temp1_);
      other.second_ = std::move(temp2_);
    }

    constexpr void copy(const compressed_pair<T1, T2, false>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      second_ = other.second();
      first_ = other.first();
    }

    compressed_pair() = default;

    template<typename... Args>
    constexpr compressed_pair(detail::FirstZeroSecondArgs, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : first_(), second_(std::forward<Args>(args)...) {
    }

    template<typename U1, typename... Args>
    constexpr compressed_pair(detail::FirstOneSecondArgs, U1&& f, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1, U1&&>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : first_(std::forward<U1>(f)), second_(std::forward<Args>(args)...) {
    }

    constexpr T1& first() noexcept { return first_; }
    constexpr const T1& first() const noexcept { return first_; }

    constexpr T2& second() noexcept { return second_; }
    constexpr const T2& second() const noexcept { return second_; }
  };

  template<typename T1, typename T2>
  class compressed_pair<T1, T2, true> final : private T1 {
    T2 second_;

  public:
    constexpr void swap(compressed_pair<T1, T2>& other) noexcept(
      std::is_nothrow_move_assignable_v<T1>&& std::is_nothrow_move_assignable_v<T2>)
    {
      T2 tmp2 = std::move(second_);
      second_ = std::move(other.second_);
      other.second_ = std::move(tmp2);
    }

    constexpr void copy(const compressed_pair<T1, T2>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      second_ = other.second();
    }

    compressed_pair() : T1() {}

    template<typename... Args>
    constexpr compressed_pair(detail::FirstZeroSecondArgs, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : T1(), second_(std::forward<Args>(args)...) {
    }

    template<typename U1, typename... Args>
    constexpr compressed_pair(detail::FirstOneSecondArgs, U1&& f, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1, U1&&>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : T1(std::forward<U1>(f)), second_(std::forward<Args>(args)...) {
    }

    constexpr T1& first() noexcept { return static_cast<T1&>(*this); }
    constexpr const T1& first() const noexcept { return static_cast<T1 const&>(*this); }

    constexpr T2& second() noexcept { return second_; }
    constexpr const T2& second() const noexcept { return second_; }
  };

  template<typename Alloc, typename InputPtr, typename DestPtr, typename SizeType>
  DestPtr uninitialized_copy_n(Alloc& alloc, InputPtr src, SizeType count, DestPtr dest)
    noexcept(
      std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type> || 
      std::is_trivially_copyable_v<typename std::allocator_traits<Alloc>::value_type>
    )
  {
    using traits = std::allocator_traits<Alloc>;
    using value_type = typename traits::value_type;
    using raw_ptr = typename Alloc::value_type*;

    if constexpr (std::is_trivially_copyable_v<value_type>) {
      memcpy(static_cast<void*>(dest), static_cast<const void*>(src), static_cast<size_t>(count) * sizeof(value_type));
      return dest + count;
    }
    else {
      size_t constructed = 0;
      try {
        for (SizeType i = 0; i < count; ++i) {
          traits::construct(alloc, dest + i, src[i]);
          ++constructed;
        }
        return dest + count;
      }
      catch (...) {
        for (size_t j = 0; j < constructed; ++j) {
          traits::destroy(alloc, dest + (constructed - 1 - j));
        }
        throw;
      }
    }
  }

  template<typename Alloc, typename InputIt, typename DestIt, typename SizeType>
  DestIt initialized_copy_n(Alloc& alloc, InputIt src, SizeType count, DestIt dest)
    noexcept(
      std::is_nothrow_copy_assignable_v<typename std::allocator_traits<Alloc>::value_type> ||
      std::is_trivially_copyable_v<typename std::allocator_traits<Alloc>::value_type>
      )
  {
    using traits = std::allocator_traits<Alloc>;
    using value_type = typename traits::value_type;
    using raw_ptr = typename Alloc::value_type*;

    if constexpr (std::is_trivially_copyable_v<value_type>) {
      memcpy(static_cast<void*>(dest), static_cast<const void*>(src), static_cast<size_t>(count) * sizeof(value_type));
      return dest + count;
    }
    else if constexpr (std::is_nothrow_copy_assignable_v<typename std::allocator_traits<Alloc>::value_type>)
    {
      for (SizeType i = 0; i < count; ++i) {
        dest[i] = src[i];
      }
      return dest + count;
    }
    else {
      size_t constructed = 0;
      try {
        for (SizeType i = 0; i < count; ++i) {
          dest[i] = src[i];
          ++constructed;
        }
        return dest + count;
      }
      catch (...) {
        for (size_t j = 0; j < constructed; ++j) {
          traits::destroy(alloc, dest + (constructed - 1 - j));
        }
        throw;
      }
    }
  }

  template<typename Alloc, typename DestPtr, typename SizeType>
  void uninitialized_default_construct_n(Alloc& a, DestPtr p, SizeType n)
    noexcept(std::is_nothrow_default_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
  {
    using traits = std::allocator_traits<Alloc>;
    using value_type = typename traits::value_type;

    constexpr bool VTrivial = std::is_trivially_default_constructible_v<value_type>;
    if constexpr (VTrivial)
    {
      std::memset(static_cast<void*>(p), 0, static_cast<size_t>(n) * sizeof(value_type));
    }
    else if constexpr (std::is_nothrow_default_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
    {
      for (SizeType i = 0; i < n; ++i) {
        traits::construct(a, p + i);
      }
    }
    else {
      size_t constructed = 0;
      try {
        for (SizeType i = 0; i < n; ++i) {
          traits::construct(a, p + i);
          ++constructed;
        }
      }
      catch (...) {
        for (size_t j = 0; j < constructed; ++j) {
          traits::destroy(a, p + (constructed - j));
        }
        throw;
      }
    }
  }

  template<typename T1, typename T2, bool B>
  constexpr void swap(compressed_pair<T1, T2, B>& first,
    compressed_pair<T1, T2, B>& second)
    noexcept(noexcept(std::declval<compressed_pair<T1, T2, B>&>().swap(
      std::declval<compressed_pair<T1, T2, B>&>())))
  {
    first.swap(second);
  }
}

