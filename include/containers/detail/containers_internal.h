#pragma once 
#include <type_traits>
#include <type_traits.h>
#include <xmemory>
#include <utility.h>
#ifdef DEBUG
#define ITER_DEBUG_WRAP(iterator_name, pointer) iterator_name(this, pointer) 
#else
#define ITER_DEBUG_WRAP(iterator_name, pointer) iterator_name(pointer) 
#endif // DEBUG

namespace voxory
{
  namespace containers {
    namespace internal {
      using utility::unfancy;

      struct no_allocator {};
      struct inlined {};

      template<typename Policy, typename = void>
      struct get_allocator_type {
        using type = no_allocator;
      };

      template<typename Policy>
      struct get_allocator_type<Policy, std::void_t<typename Policy::allocator_type>> {
        using type = typename Policy::allocator_type;
      };

      template<typename Policy, typename = void>
      struct get_inlined_size : std::integral_constant<typename Policy::size_type, 0> {};

      template<typename Policy>
      struct get_inlined_size<Policy, std::void_t<typename Policy::inlined_size>> : Policy::inlined_size {};

      struct empty_data {};
      struct move_tag {};
      struct copy_tag {};

      template<typename Alloc>
      class construct_helper {
      public:
        using allocator_type = Alloc;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using value_type = typename allocator_traits::value_type;
        using pointer = typename allocator_traits::pointer;
        construct_helper(Alloc& alloc, pointer first) noexcept : alloc_(std::ref(alloc)), first_(first), current_(first) {};
        ~construct_helper() {
          if constexpr (type_traits::has_destroy<allocator_type, pointer>::value)
          {
            for (; first_ < current_; first_++)
            {
              alloc_.get().destroy(first_);
            }
          }
          else {
            for (; first_ < current_; first_++)
            {
              std::destroy_at(first_);
            }
          }
        }

        template<typename... Args>
        CONSTEXPR void construct_one(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) {
          allocator_traits::construct(alloc_.get(), current_, std::forward<Args>(args)...);
          current_++;
        }

        CONSTEXPR void release() noexcept {
          first_ = current_;
        }

        NODISCARD CONSTEXPR pointer current() noexcept {
          return current_;
        }

      private:
        std::reference_wrapper<Alloc> alloc_;
        pointer first_;
        pointer current_;
      };

      template<typename Alloc>
      class assign_helper {
      public:
        using allocator_type = Alloc;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using value_type = typename allocator_traits::value_type;
        using pointer = typename allocator_traits::pointer;
        assign_helper(Alloc& alloc, pointer first) noexcept : alloc_(std::ref(alloc)), first_(first), current_(first) {};
        ~assign_helper() {
          for (; first_ != current_; first_++) {
            if constexpr (type_traits::has_destroy<allocator_type, pointer>::value)
            {
              alloc_.get().destroy(alloc_.get(), first_);
            }
            else {
              std::destroy_at(first_);
            }
          }
        }


        CONSTEXPR void assign_copy_one(value_type& val) noexcept(std::is_nothrow_copy_assignable_v<value_type>) {
          *current_ = val;
          current_++;
        }

        CONSTEXPR void assign_move_one(value_type& val) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
          *current_ = std::move(val);
          current_++;
        }

        CONSTEXPR void release() noexcept {
          first_ = current_;
        }

        NODISCARD CONSTEXPR pointer current() noexcept {
          return current_;
        }

      private:
        std::reference_wrapper<Alloc> alloc_;
        pointer first_;
        pointer current_;
      };

      template<typename T>
      class cleanup_guard {
      public:
        cleanup_guard(T* obj) : obj_(obj) {};
        ~cleanup_guard() {
          if (obj_)
          {
            obj_->cleanup();
          }
        };
        void release() {
          obj_ = nullptr;
        };
      private:
        T* obj_;
      };

      template<typename Wrapper>
      NODISCARD CONSTEXPR auto unwrap(Wrapper&& wrap) noexcept -> decltype(auto) {
        if constexpr (type_traits::is_reference_wrapper<Wrapper>::value)
        {
          return std::addressof(wrap.get());
        }
        else if constexpr (std::is_pointer<Wrapper>::value) {
          return wrap;
        }
        else if constexpr (type_traits::has_deref<Wrapper>::value) {
          return std::addressof(*wrap);
        }
        else if constexpr (type_traits::is_optional<Wrapper>::value) {
          return std::addressof(*wrap);
        }
        else {
          static_assert(type_traits::always_false<Wrapper>, "unwrap: unsupported wrapper type");
        }
      }
      // TODO: potential issues with custom allocators (construct/destroy).
      // Need to review the noexcept logic or switch to default construction.
      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_move(const FwdIt first, const FwdIt end, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_move_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = unfancy(unwrap(end));
          std::size_t count = static_cast<std::size_t>(src_end - src_raw);
          std::size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer unwrapped_end = unfancy(unwrap(end));
          for (; it != unwrapped_end; ++it) {
            c.construct_one(std::move(*it));
          }
          c.release();
          dest += end - first;
          return dest;
        }
      }

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_copy(const FwdIt first, const FwdIt end, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = unfancy(unwrap(end));
          std::size_t count = static_cast<std::size_t>(src_end - src_raw);
          std::size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer unwrapped_end = unfancy(unwrap(end));
          for (; it != unwrapped_end; ++it) {
            c.construct_one(*it);
          }
          c.release();
          dest += end - first;
          return dest;
        }
      }
      //works only with trivial types
      template<typename T>
      CONSTEXPR bool is_zeroed(const T& val) {
        constexpr T zero{};
        return std::memcmp(&val, &zero, sizeof(T));
      }

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_fill(const FwdIt first, const FwdIt end, const typename std::allocator_traits<Alloc>::value_type& val, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memset_value_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = unfancy(unwrap(end));
          std::size_t count = static_cast<std::size_t>(src_end - src_raw);
          std::size_t bytes = count * sizeof(value_type);
          if (is_zeroed(val))
          {
            std::memset(src_raw, 0, bytes);
            return src_raw + count;
          }
          else {
            std::memset(src_raw, &val, bytes);
            return src_raw + count;
          }
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(unwrap(first)));
          pointer it = unfancy(unwrap(first));
          pointer unwrapped_end = unfancy(unwrap(end));
          for (; it != unwrapped_end; ++it) {
            c.construct_one(val);
          }
          c.release();
          return c.current();
        }
      }

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_default_construct(const FwdIt first, const FwdIt end, Alloc& alloc)
        noexcept(type_traits::use_memset_value_construct_v<FwdIt> || std::is_nothrow_default_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memset_value_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = unfancy(unwrap(end));
          std::size_t count = static_cast<std::size_t>(src_end - src_raw);
          std::size_t bytes = count * sizeof(value_type);
          std::memset(src_raw, 0, bytes);
          return src_raw + count;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(unwrap(first)));
          pointer it = unfancy(unwrap(first));
          pointer unwrapped_end = unfancy(unwrap(end));
          for (; it != unwrapped_end; ++it) {
            c.construct_one();
          }
          c.release();
          return c.current();
        }
      }

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_copy_n(const FwdIt first, size_t count, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = src_raw + count;
          size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            c.construct_one(*it);
          }
          c.release();
          dest += count;
          return dest;
        }
      }
      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_move_n(const FwdIt first, size_t count, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_move_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = src_raw + count;
          size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            c.construct_one(std::move(*it));
          }
          c.release();
          dest += count;
          return dest;
        }
      }

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_fill_n(const FwdIt first, size_t count, const typename std::allocator_traits<Alloc>::value_type& val, Alloc& alloc)
        noexcept(type_traits::use_memset_value_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memset_value_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          std::size_t bytes = count * sizeof(value_type);
          if (is_zeroed(val))
          {
            std::memset(src_raw, 0, bytes);
            return src_raw + count;
          }
          else {
            std::memset(src_raw, val, bytes);
            return src_raw + count;
          }
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(unwrap(first)));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            c.construct_one(val);
          }
          c.release();
          return c.current();
        }
      }

      template<typename Alloc, typename FwdIt, typename... Args>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer uninitialized_default_construct_n(const FwdIt first, size_t count, Alloc& alloc)
        noexcept(type_traits::use_memset_value_construct_v<FwdIt> || std::is_nothrow_default_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memset_value_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          std::size_t bytes = count * sizeof(value_type);
          std::memset(src_raw, 0, bytes);
          return src_raw + count;
        }
        else {
          // generic safe path for forward iterators
          construct_helper<Alloc> c(alloc, unfancy(unwrap(first)));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            c.construct_one();
          }
          c.release();
          return c.current();
        }
      }

      template<typename Alloc>
      class realloc_guard {
      public:
        using allocator_type = Alloc;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using pointer = typename allocator_traits::pointer;
        realloc_guard(allocator_type& alloc, pointer first, size_t size) noexcept
          : alloc_(std::ref(alloc)), first_(first), size_(size) {
        }

        ~realloc_guard() {
          if (first_ != nullptr) {
            alloc_.get().deallocate(first_, size_);
          }
        }

        CONSTEXPR void release() noexcept {
          first_ = nullptr;
        };

      private:
        std::reference_wrapper<Alloc> alloc_;
        pointer first_;
        size_t size_;
      };

      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer copy_assign_n(const FwdIt first, size_t count, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = src_raw + count;
          size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          assign_helper<Alloc> ah(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            ah.assign_copy_one(*it);
          }
          ah.release();
          dest += count;
          return dest;
        }
      }
      template<typename Alloc, typename FwdIt>
      NODISCARD CONSTEXPR typename std::allocator_traits<Alloc>::pointer move_assign_n(const FwdIt first, const size_t count, typename std::allocator_traits<Alloc>::pointer dest, Alloc& alloc)
        noexcept(type_traits::use_memmove_copy_construct_v<FwdIt> || std::is_nothrow_copy_constructible_v<typename std::allocator_traits<Alloc>::value_type>)
      {
        using value_type = typename std::allocator_traits<Alloc>::value_type;
        using it_value = typename std::iterator_traits<FwdIt>::value_type;
        using pointer = typename std::allocator_traits<Alloc>::pointer;
        static_assert(std::is_same_v<it_value, value_type>, "Iterator value_type must match allocator value_type");

        if constexpr (type_traits::use_memmove_copy_construct_v<FwdIt>) {
          // assume contiguous / pointer-like: get raw pointers
          pointer src_raw = unfancy(unwrap(first));
          pointer src_end = src_raw + count;
          size_t bytes = count * sizeof(value_type);
          std::memmove(unfancy(dest), src_raw, bytes);
          dest += count;
          return dest;
        }
        else {
          // generic safe path for forward iterators
          assign_helper<Alloc> ah(alloc, unfancy(dest));
          pointer it = unfancy(unwrap(first));
          pointer end = it + count;
          for (; it != end; ++it) {
            ah.assign_move_one(*it);
          }
          ah.release();
          dest += count;
          return dest;
        }
      }
    }
  }
}