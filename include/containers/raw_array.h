#pragma once
#include <utility.h>
#include <type_traits.h>
#include <stdexcept>
#include <platform/platform.h>
#include <containers/ring_buffer.h>

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

    // опційно: зручний alias (необов'язково)
    template<typename Pointer, typename Alloc, typename... Args>
    inline constexpr bool IsNoexceptConstructRange_v = IsNoexceptConstructRange<Pointer, Alloc, Args...>;

    template<typename Policy, typename = void>
    struct is_container_data_inlined : std::false_type {};

    template<typename Policy>
    struct is_container_data_inlined<Policy, std::void_t<typename Policy::is_data_inlined>> : std::bool_constant<typename Policy::is_data_inlined::value> {};

    struct no_allocator {};
    struct inlined {};

    template<typename Policy, typename = void>
    struct get_allocator {
      using type = no_allocator;
    };

    template<typename Policy>
    struct get_allocator<Policy, std::void_t<typename Policy::allocator_type>> {
      using type = typename Policy::allocator_type;
    };

    template<typename Policy, typename = void>
    struct get_inlined_size : std::integral_constant<typename Policy::size_type, 0> {};

    template<typename Policy>
    struct get_inlined_size<Policy, std::void_t<typename Policy::inlined_size>> : Policy::inlined_size {};

    struct empty_data {};
  }
  template<typename Policy>
  class raw_array {
    using FirstOneSecondArgs = utility::detail::FirstOneSecondArgs;
    using FirstZeroSecondArgs = utility::detail::FirstZeroSecondArgs;
  public:
    using value_type = typename Policy::value_type;
    using pointer = typename Policy::pointer;
    using size_type = typename Policy::size_type;
    using cleanup_guard = internal::cleanup_guard<raw_array>;
    using trivial_traits = type_traits::trivial_traits<value_type>;

    static_assert(std::is_same_v<value_type, typename allocator_traits::value_type>, "");
    static_assert(std::is_object_v<value_type>, "");

    constexpr bool is_data_inlined = detail::is_container_data_inlined<Policy>::value;
    using grow_policy = std::conditional_t<is_data_inlined, detail::inlined, typename Policy::grow_policy>;
    using allocator_type = detail::get_allocator<Policy>;
    //default std allocator to simplify logic
    using allocator_traits = std::conditional_t<std::is_same_v<allocator_type, detail::no_allocator>, std::allocator<value_type>, std::allocator_traits<allocator_type>>;
    using inlined_array_size = detail::get_inlined_size<Policy>;
    using inlined_array_type = std::conditional_t<inlined_array_size::value == 0, detail::empty_data, value_type[inlined_array_size::value]>;

    using pair = std::conditional_t<is_data_inlined, utility::compressed_pair<allocator_type, std::pair<inlined_array_type, pointer>>,
                                                     utility::compressed_pair<allocator_type, std::pair<pointer, pointer>>>;

    raw_array(size_t count, value_type&& val, allocator_type& alloc = allocator_type()) 
      noexcept(noexcept(allocate_buffer_non_zero(std::declval<size_t>())) 
      && noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
      : pair_(FirstOneSecondArgs{}, alloc) {
      if constexpr (!is_data_inlined) {
        capacity_ = count;
        allocate_buffer_non_zero(count);
      }
      construct_range(count, std::forward<value_type>(val));
    };
    
    raw_array(size_t count, allocator_type& alloc = allocator_type()) 
      noexcept(noexcept(allocate_buffer_non_zero(std::declval<size_t>()))
        && noexcept(construct_range(std::declval<size_t>(), std::declval<value_type&&>())))
      : pair_(FirstOneSecondArgs{}, alloc)
    {
      if constexpr (!is_data_inlined) {
        capacity_ = count;
        allocate_buffer_non_zero(count);
      }
      construct_range(count);
    };

    raw_array(const raw_array& o) noexcept() : pair_(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o.pair_.first())) {
      if constexpr (!is_data_inlined) {
        capacity_ = o.capacity_;
        allocate_buffer_non_zero(o.capacity_);
        construct_range(o.capacity_, o.pair_.second().first, o.pair_.second().second);
      }
      else {
        constexpr size_type count = std::min(capacity_, o.capacity_);
        construct_range(count, o.pair_.second().first, o.pair_.second().first + count);
        if (capacity_ - count !=0)
        {
          internal::uninitialized_default_construct_n(o.pair_.second().first + count, capacity_ - count, pair_.first());
        }
      }
    };

    raw_array(raw_array&& o) noexcept() : pair_(FirstOneSecondArgs{}, std::move(o.pair_.first())), capacity_(o.capacity_) {
      pair_.second().first  = o.pair_.second().first;
      pair_.second().second = o.pair_.second().second;
      o.pair_.second().first  = nullptr;
      o.pair_.second().second = nullptr;
    };

    ~raw_array() {
      cleanup();
    }

    raw_array& operator=(raw_array&& o) noexcept() {
      if (this == &o) return *this;
      auto& al = get_allocator();
      auto& o_al = o.get_allocator();

      auto& data = pair_.second();
      auto& o_data = o.pair_.second();

      cleanup();
      if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
        al = std::move(o_al);
        data = std::move(o_data);
        o_data = { nullptr, nullptr };
        capacity_ = o.capacity_;
        o.capacity_ = 0;
        return *this;
      }
      else if constexpr (allocator_traits::is_always_equal::value) {
        data = std::move(o_data);
        capacity_ = o.capacity_;
        o_data = { nullptr, nullptr };
        o.capacity_ = 0;
        return *this;
      }
      else
      {
        if (al == o_al)
        {
          data = std::move(o_data);
          capacity_ = o.capacity_;
          o_data = { nullptr, nullptr };
          o.capacity_ = 0;
        }
        else
        {

        }
        return *this;
      }
    };

    raw_array& operator=(const raw_array& o);
    raw_array& operator[](size_t idx);

    _CONSTEXPR20 void reserve(size_t count) noexcept() {
      auto& data = pair_.second();
      auto& first = data.first;
      auto& second = data.second;
      allocate_buffer_non_zero(count);
      construct_range(count);
    };
    
    _NODISCARD _CONSTEXPR20 pointer data() noexcept {
      return pair_.second().first;
    };
    
    _NODISCARD _CONSTEXPR20 size_type capacity() noexcept {
      return capacity_;
    };

    template<typename = std::enable_if_t<!is_data_inlined>>
    _NODISCARD _CONSTEXPR20 allocator_type& get_allocator() noexcept {
      return pair_.first();
    };

  private:
    _CONSTEXPR20 void cleanup() noexcept {
      auto& data = pair_.second();
      auto& first = data.first;
      auto& last = data.second;

      if (first != nullptr)
      {
        destruct_range(first, last);
        if constexpr (!is_data_inlined)
        {
          deallocate_buffer();
          first = nullptr;
        }

        last = nullptr;
        capacity_ = 0;
      }
    };

    template<typename = std::enable_if_t<!is_data_inlined>>
    _CONSTEXPR20 void allocate_buffer(size_t n) noexcept(is_data_inlined || ) {
        auto& alloc = pair_.first();
        auto& data = pair_.second();
        pointer new_data = alloc.allocate(n);
        data.first = new_data;
        data.second = new_data;
    };

    template<typename = std::enable_if_t<!is_data_inlined>>
    _CONSTEXPR20 void allocate_buffer_non_zero(size_t n) noexcept() {
      if constexpr (!is_data_inlined)
      {
        auto& data = pair_.second();

        if (n <= 0)
        {
          data.first = nullptr;
          data.second = nullptr;
          return;
        }

        allocate_buffer(n);
      }
    };

    template<typename = std::enable_if_t<!is_data_inlined>>
    _CONSTEXPR20 void deallocate_buffer() noexcept {
      auto& alloc = pair_.first();
      auto& data  = pair_.second();
      alloc.deallocate(data.first, capacity_);
    };

    //TODO: check on existing of destroy, use default_destruct
    _CONSTEXPR20 void destruct_range(pointer first, pointer end) noexcept {
      if constexpr (!trivial_traits::is_trivially_destructible::value) {
        for (; first < end; first++)
          allocator_traits::destroy(first);
      }
    };

    template<typename... Args>
    _CONSTEXPR20 void construct_range(size_t count, Args&&... args) noexcept(detail::IsNoexceptConstructRange_v<pointer, allocator_type, Args...>) {
      // sizeof...(Args) == 0 = default construct
      // sizeof...(Args) == 1 = fill_n with value
      // sizeof...(Args) == 2 = copy construct from src(start, end)
      auto& data = pair_.second();
      auto& alloc = pair_.first();

      auto& first = data.first;
      auto& last = data.second;
      pointer end = first + capacity_;
      constexpr uint8_t args_count = sizeof...(Args);
      if (count != 0)
      {
        if constexpr (!is_data_inlined)
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
        else {
          if constexpr (args_count == 0) {
            last = internal::uninitialized_default_construct(first, last, alloc);
          }
          else if constexpr (args_count == 1) {
            last = internal::uninitialized_fill_n(first, capacity_, std::forward<Args>(args)..., alloc);
          }
          else if constexpr (args_count == 2) {

            size_type fuck = std::min
            last = internal::uninitialized_copy(std::forward<Args>(args)..., first, alloc);
          }
        }
      }
    };

    
  protected:
    utility::compressed_pair<allocator_type, std::pair<pointer, pointer>> pair_;
    static constexpr size_type static_cap = inlined_array_size::value;
    size_type capacity_ = (static_cap == 0 ? 0 : static_cap);
  };
}