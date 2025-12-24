#pragma once
#include <type_traits>
#include <platform/platform.h>

namespace type_traits {
  template<typename It>
  using iter_value_t = typename std::iterator_traits<It>::value_type;
  
  template<typename It>
  using iter_reference_t = typename std::iterator_traits<It>::reference;
  
  template<typename It, typename = void>
  struct is_contiguous_like : std::false_type {};
  
  template<typename It>
  struct is_contiguous_like<It, std::void_t<typename std::iterator_traits<It>::pointer>> {
  private:
    using pointer_t = typename std::iterator_traits<It>::pointer;
    using value_t = typename std::iterator_traits<It>::value_type;
  public:
    static constexpr bool value =
      std::is_pointer_v<It> ||
      std::is_same_v<pointer_t, value_t*>;
  };
  
  template<typename It>
  using is_contiguous_iterator = std::bool_constant<is_contiguous_like<It>::value>;
  template<typename It>
  inline constexpr bool is_contiguous_iterator_v = is_contiguous_like<It>::value;

  template <typename It>
  constexpr bool use_zero_memset_value_construct_v =
    std::conjunction_v<
    is_contiguous_iterator<It>,
    std::is_scalar<iter_value_t<It>>,
    std::negation<std::is_volatile<std::remove_reference_t<iter_reference_t<It>>>>,
    std::negation<std::is_member_pointer<iter_value_t<It>>>
    >;

  template<typename T>
  struct is_character : std::false_type {};
  template<>
  struct is_character<char> : std::true_type {};
  template<>
  struct is_character<char8_t> : std::true_type {};
  template<>
  struct is_character<uint8_t> : std::true_type {};
  template<>
  struct is_character<int8_t> : std::true_type {};

  template<typename T>
  struct is_boolean : std::false_type {};
  
  template<>
  struct is_boolean<bool> : std::true_type {};
  
  template <typename It>
  constexpr bool use_memset_value_construct_v =
    std::conjunction_v<
    is_contiguous_iterator<It>,
    std::is_scalar<iter_value_t<It>>,
    std::disjunction<is_character<iter_value_t<It>>, is_boolean<iter_value_t<It>>>,
    std::negation<std::is_volatile<std::remove_reference_t<iter_reference_t<It>>>>,
    std::negation<std::is_member_pointer<iter_value_t<It>>>
    >;

  template <typename It>
  constexpr bool use_memcpy_copy_construct_v =
    std::conjunction_v<
    is_contiguous_iterator<It>,
    std::is_trivially_copyable<iter_value_t<It>>,
    std::negation<std::is_volatile<std::remove_reference_t<iter_reference_t<It>>>>
    >;

  template <typename It>
  constexpr bool use_memmove_copy_construct_v = use_memcpy_copy_construct_v<It>;

  template <typename It>
  constexpr bool use_memset_bytewise_v =
    std::conjunction_v<
    is_contiguous_iterator<It>,
    std::is_trivially_copyable<iter_value_t<It>>,
    std::negation<std::is_volatile<std::remove_reference_t<iter_reference_t<It>>>>
    >;

  template<typename...>
  constexpr bool always_false = false;
 
  template<typename T>
  struct trivial_traits {
  public:
    using is_trivially_destructible = std::is_trivially_constructible<T>;
    using is_trivially_constructible = std::is_trivially_destructible<T>;
    using is_trivially_copyable = std::is_trivially_copyable<T>;
    using is_trivially_copy_constructible = std::is_trivially_copy_constructible<T>;
    using is_trivially_move_constructible = std::is_trivially_move_constructible<T>;
    using is_trivially_copy_assignable = std::is_trivially_copy_assignable<T>;
    using is_trivially_move_assignable = std::is_trivially_move_assignable<T>;
  };
  
  template<typename T, typename = void>
  struct has_deref : std::false_type {};
  
  template<typename T>
  struct has_deref<T, std::void_t<decltype(*std::declval<T&>())>> : std::true_type {};
  
  template<typename T, typename = void>
  struct is_unwrappable : std::false_type {};
  
  template<typename T>
  struct is_unwrappable<T, std::void_t<decltype(std::declval<T&>()._Unwrap)>> : std::true_type {};

  template<typename T>
  struct is_reference_wrapper : std::false_type {};

  template<typename U>
  struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type{};

  template<typename T, typename...>
  NODISCARD CONSTEXPR auto first_arg(T&& arg, ...) noexcept -> decltype(auto) {
    return std::forward<T>(arg);
  }

  template<typename Alloc, typename Pointer, typename = void>
  struct has_destroy : std::false_type {};

  template<typename Alloc, typename Pointer>
  struct has_destroy<
    Alloc,
    Pointer,
    std::void_t<decltype(std::declval<Alloc&>().destroy(std::declval<Pointer>()))>
  > : std::true_type {
  };

  template<typename Alloc, typename Pointer>
  constexpr bool has_destroy_v = has_destroy<Alloc, Pointer>::value;

  template<typename Alloc, typename Pointer, typename... Args>
  struct has_construct {
  private:
    template<typename A, typename P, typename... As>
    static auto test(int) -> decltype(
      std::declval<A&>().construct(std::declval<P>(), std::declval<As>()...), 
      std::true_type{}   
      );

    template<typename, typename, typename...>
    static std::false_type test(...);

  public:
    static constexpr bool value = decltype(test<Alloc, Pointer, Args...>(0))::value;
  };

  template<typename Alloc, typename Pointer, typename... Args>
  constexpr bool has_construct_v = has_construct<Alloc, Pointer, Args...>::value;

  template<std::size_t N, typename... Args>
  decltype(auto) nth_arg(Args&&... args) {
    return std::get<N>(std::forward_as_tuple(std::forward<Args>(args)...));
  }
} // namespace type_traits
