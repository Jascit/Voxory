#pragma once
#define NOMINMAX
#include <utility.h>
#include <type_traits.h>
#include <stdexcept>
#include <platform/platform.h>
#include <containers/ring_buffer.h>
#include <containers/containers_internal.h>

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
  }
  template<typename Policy>
  class heap_array {
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

    heap_array(const heap_array& o) noexcept() :
      pair_(FirstOneSecondArgs{}, allocator_traits::select_on_container_copy_construction(o.pair_.first())), capacity_(o.capacity_) {
      construct_range(o.capacity_, o.pair_.second().first, o.pair_.second().second);
    };

    heap_array(heap_array&& o) noexcept() : pair_(FirstOneSecondArgs{}, std::move(o.pair_.first())), capacity_(o.capacity_) {
      pair_.second().first = o.pair_.second().first;
      pair_.second().second = o.pair_.second().second;
      o.pair_.second().first = nullptr;
      o.pair_.second().second = nullptr;
      o.capacity_ = 0;
    };

    ~heap_array() {
      cleanup();
    }

    heap_array& operator=(heap_array&& o) noexcept() {
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

    heap_array& operator=(const heap_array& o);
    heap_array& operator[](size_t idx) noexcept;
    heap_array& operator[](size_t idx) const noexcept;

    CONSTEXPR void reserve(size_t count) noexcept() {
      auto& data = pair_.second();
      auto& first = data.first;
      auto& second = data.second;
      allocate_buffer_non_zero(count);
      construct_range(count);
    };

    NODISCARD CONSTEXPR pointer data() noexcept {
      return pair_.second().first;
    };

    NODISCARD CONSTEXPR size_type capacity() noexcept {
      return capacity_;
    };

    NODISCARD CONSTEXPR allocator_type& get_allocator() noexcept {
      return pair_.first();
    };

    NODISCARD CONSTEXPR size_type size() const noexcept {
      return static_cast<size_type>(pair_.second().second - pair_.second().first);
    };


  private:
    CONSTEXPR void cleanup() noexcept {
      auto& data = pair_.second();
      auto& first = data.first;
      auto& last = data.second;

      if (first != nullptr)
      {
        destruct_range(first, last);
        deallocate_buffer();
        first = nullptr;
        last = nullptr;
        capacity_ = 0;
      }
    };

    CONSTEXPR void allocate_buffer(size_t n) noexcept( ) {
        auto& alloc = pair_.first();
        auto& data = pair_.second();
        pointer new_data = alloc.allocate(n);
        data.first = new_data;
        data.second = new_data;
    };

    CONSTEXPR void allocate_buffer_non_zero(size_t n) noexcept() {
      auto& data = pair_.second();

      if (n <= 0)
      {
        data.first = nullptr;
        data.second = nullptr;
        return;
      }

      allocate_buffer(n);
    };

    CONSTEXPR void deallocate_buffer() noexcept {
      auto& alloc = pair_.first();
      auto& data  = pair_.second();
      alloc.deallocate(data.first, capacity_);
    };

    //TODO: check on existing of destroy, use default_destruct
    CONSTEXPR void destroy_range(pointer first, pointer end) noexcept {
      if constexpr (!trivial_traits::is_trivially_destructible::value) {
        auto& alloc = pair_.first();
        for (; first < end; first++) {
          if constexpr (type_traits::has_destroy<allocator_type, pointer>::value) {
            alloc.destroy(first);
          } else {
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
      auto& data = pair_.second();
      auto& alloc = pair_.first();

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

    CONSTEXPR void new_buffer(size_type new_capacity) noexcept() {
      auto& data = pair_.second();
      auto& alloc = pair_.first();
      auto& first = data.first;
      auto& last = data.second;

      pointer new_data = alloc.allocate(new_capacity);
      pointer new_last = new_data;
      internal::realloc_guard<allocator_type> guard(alloc, new_data, new_data + new_capacity);
      if (first != nullptr)
      {
        //guard, if exception occurs during move: destroy new_data, leave old intact
        new_last = internal::uninitialized_move(first, last, new_data, alloc);
        destroy_range(first, last);
        deallocate_buffer();
      }
      first = new_data;
      last = new_last;
      capacity_ = new_capacity;
      guard.release();
    };

    CONSTEXPR void new_buffer(size_type new_capacity, pointer src, size_type new_size) noexcept() {
      auto& data = pair_.second();
      auto& alloc = pair_.first();
      auto& first = data.first;
      auto& last = data.second;

      pointer new_data = alloc.allocate(new_capacity);
      pointer new_last = new_data;
      internal::realloc_guard<allocator_type> guard(alloc, new_data, new_data + new_capacity);
      if (first != nullptr)
      {
        //guard, if exception occurs during move: destroy new_data, leave old intact
        destroy_range(first, last);
        deallocate_buffer();
        new_last = internal::uninitialized_copy(src, src+new_size, new_data, alloc);
      }

      first = new_data;
      last = new_last;
      capacity_ = new_capacity;
      guard.release();
    };

    NODISCARD CONSTEXPR void assign_copy(heap_array& o) noexcept(std::is_nothrow_copy_assignable_v<value_type>) {
      auto& data  = pair_.second();
      auto& alloc = pair_.first();

      auto& first = data.first;
      auto& last  = data.second;
      auto& end   = first + capacity_;
      
      auto o_data  = o.pair_.second();
      auto o_first = o_data.first;
      auto o_last  = o_data.second;
      auto o_end   = o_first + o.capacity_;

      size_type o_cap = o.capacity_;
      size_type new_size = o.size_;
      size_type size = last-first;//alive objects && default constructed 
      
      //allocate new buffer, copy construct all
      if (new_size > capacity_)
      {
        new_buffer(o_cap, o_first, new_size);
      }
      //copy assign within existing buffer, copy construct rest
      else if (new_size > size)
      {
        size_type rest = new_size - size;
        pointer dest = first;
        for (; o_first < o_first + size; o_first++) {
          *dest = *o_first;
          dest++;
        }
        internal::uninitialized_copy_n(o_first, rest, dest, alloc);
      }
      //copy assign within existing buffer, destroy rest
      else {

      }
    }

    NODISCARD CONSTEXPR void assign_move(heap_array& o) noexcept(std::is_nothrow_assignable_construcluible_v<value_type>) {
      
    }
    
  protected:
    utility::compressed_pair<allocator_type, std::pair<pointer, pointer>> pair_;
    size_type capacity_;
  };
}