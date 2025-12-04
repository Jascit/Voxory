#pragma once
#include <vector>
#include <platform/platform.h>
#include <atomic>
namespace containers {
  namespace debug {
    std::_Container_base12
    namespace detail {
      struct container_gen {
        container_gen() : generation_(0) {};
        std::atomic<uint64_t> generation_;
      };
      class container_base_dbg {
      public:
        virtual ~container_base_dbg() = default;
        CONSTEXPR void update_generation() noexcept {
          gen_.generation_.fetch_add(1, std::memory_order_release);
        }
        NODISCARD CONSTEXPR container_gen* get_generation() noexcept {
          return &gen_;
        }
        container_gen gen_;
      };

      class container_base_rls {
      public:
        virtual ~container_base_rls() = default;
        CONSTEXPR void update_generation() noexcept {}
        NODISCARD CONSTEXPR FORCE_INLINE uint64_t get_generation() const noexcept {}
      };

      class iterator_base_dbg {
        uint64_t generation_;
        container_gen* container_gen_;
      public:
        
      };
    class iterator_base_rls;
    }
#ifdef DEBUG_ITERATORS
    using container_base = detail::container_base_dbg;
    using iterator_base = detail::iterator_base_dbg;
#else
    using container_base = detail::container_base_rls;
    using iterator_base = detail::iterator_base_rls;
#endif // DEBUG

  }
}