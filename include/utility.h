#pragma once
#include <type_traits>
#include <xmemory>
#include <type_traits.h>

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

  public:
    T2 second_;
    constexpr void swap(compressed_pair<T1, T2, false>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      T1 temp1_ = std::move(first_);
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
      std::is_nothrow_constructible_v<T1>&&
      std::is_nothrow_constructible_v<T2, Args&&...>
      )
      : first_(), second_(std::forward<Args>(args)...) {
    }

    template<typename U1, typename... Args>
    constexpr compressed_pair(detail::FirstOneSecondArgs, U1&& f, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T1, U1&&>&&
      std::is_nothrow_constructible_v<T2, Args&&...>)
      : first_(std::forward<U1>(f)), second_(std::forward<Args>(args)...) {
    }

    constexpr T1& first() noexcept { return first_; }
    constexpr const T1& first() const noexcept { return first_; }
  };

  template<typename T1, typename T2>
  class compressed_pair<T1, T2, true> final : private T1 {
  public:
    T2 second_;

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
    constexpr const T1& first() const noexcept { return static…_cast<T1 const&>(*this); }
  };

  template<typename Alloc, typename FirstIt, typename SecondIt>
  _CONSTEXPR20 void destroy_range(Alloc& a, FirstIt first, SecondIt last)
    noexcept(std::is_nothrow_destructible_v<typename std::iterator_traits<FirstIt>::value_type>)
  {
    using traits = std::allocator_traits<Alloc>;
    using value_type = typename traits::value_type;
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      for (auto it = first; it != last; ++it) {
        traits::destroy(a, std::addressof(*it));
      }
    }
  }
  template <class FwdIt, class LastIt>
  _CONSTEXPR20 void destroy_range(FwdIt start, LastIt last) noexcept(std::is_nothrow_destructible_v<typename std::iterator_traits<FwdIt>::value_type>);

  template<typename T>
  _CONSTEXPR20 void destroy_in_place(T& o)
    noexcept(std::is_nothrow_destructible_v<T>)
  {
    if constexpr (std::is_array_v<T>) {
      destroy_range(o, o + std::extent_v<T>);
    }
    else {
      o.~T();
    }
  }

  template <class FwdIt, class LastIt>
  _CONSTEXPR20 void destroy_range(FwdIt start, LastIt last) noexcept(std::is_nothrow_destructible_v<typename std::iterator_traits<FwdIt>::value_type>) {
    using value_type = typename std::iterator_traits<FwdIt>::value_type;
    if constexpr (!std::is_trivially_destructible_v<value_type>)
    {
      for (; start != last; ++start) {
        destroy_in_place(*start);
      }
    }
  };


  template<typename T1, typename T2, bool B>
  _CONSTEXPR20 void swap(compressed_pair<T1, T2, B>& first,
    compressed_pair<T1, T2, B>& second)
    noexcept(noexcept(std::declval<compressed_pair<T1, T2, B>&>().swap(
      std::declval<compressed_pair<T1, T2, B>&>())))
  {
    first.swap(second);
  }

  template<typename Ptr>
  _CONSTEXPR20 Ptr* unfancy(Ptr* p) noexcept {
    return p;
  }

  template<typename Ptr>
  _CONSTEXPR20 auto unfancy(Ptr p) noexcept {
    return std::addressof(*p);
  }
}

