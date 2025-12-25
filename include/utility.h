#pragma once
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
    T1 _first;

  public:
    T2 _second;
    constexpr void swap(compressed_pair<T1, T2, false>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      T1 temp1_ = std::move(_first);
      T2 temp2_ = std::move(_second);
      _second = std::move(other.second());
      _first = std::move(other.first());
      other._second = std::move(temp1_);
      other._second = std::move(temp2_);
    }

    constexpr void copy(const compressed_pair<T1, T2, false>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      _second = other.second();
      _first = other.first();
    }

    compressed_pair() = default;

    template<typename... Args>
    constexpr compressed_pair(detail::FirstZeroSecondArgs, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T1>&&
      std::is_nothrow_constructible_v<T2, Args&&...>
      )
      : _first(), _second(std::forward<Args>(args)...) {
    }

    template<typename U1, typename... Args>
    constexpr compressed_pair(detail::FirstOneSecondArgs, U1&& f, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T1, U1&&>&&
      std::is_nothrow_constructible_v<T2, Args&&...>)
      : _first(std::forward<U1>(f)), _second(std::forward<Args>(args)...) {
    }

    constexpr T1& first() noexcept { return _first; }
    constexpr const T1& first() const noexcept { return _first; }
  };

  template<typename T1, typename T2>
  class compressed_pair<T1, T2, true> final : private T1 {
  public:
    T2 _second;

    constexpr void swap(compressed_pair<T1, T2>& other) noexcept(
      std::is_nothrow_move_assignable_v<T1>&& std::is_nothrow_move_assignable_v<T2>)
    {
      T2 tmp2 = std::move(_second);
      _second = std::move(other._second);
      other._second = std::move(tmp2);
    }

    constexpr void copy(const compressed_pair<T1, T2>& other) noexcept(std::is_nothrow_copy_assignable_v<T1>&& std::is_nothrow_copy_assignable_v<T2>) {
      _second = other.second();
    }

    compressed_pair() : T1() {}

    template<typename... Args>
    constexpr compressed_pair(detail::FirstZeroSecondArgs, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : T1(), _second(std::forward<Args>(args)...) {
    }

    template<typename U1, typename... Args>
    constexpr compressed_pair(detail::FirstOneSecondArgs, U1&& f, Args&&... args) noexcept(
      std::conjunction_v<
      std::is_nothrow_constructible<T1, U1&&>,
      std::is_nothrow_constructible<T2, Args&&...>
      >)
      : T1(std::forward<U1>(f)), _second(std::forward<Args>(args)...) {
    }

    constexpr T1& first() noexcept { return static_cast<T1&>(*this); }
    constexpr const T1& first() const noexcept { return static_cast<T1 const&>(*this); }
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

