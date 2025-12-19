#pragma once
#include <platform/platform.h>
#include <utility.h>
#include <multithreading/spin_lock.h>
#include <mutex>
#include <containers/impl/list.h>

namespace voxory {
  namespace containers {
    namespace debug {
      class iterator_base_dbg;
      class container_base_dbg {
      public: 
        using list = list<iterator_base_dbg*> ;
        container_base_dbg() noexcept(std::is_nothrow_default_constructible_v<list>) = default;
        container_base_dbg(const container_base_dbg&) noexcept(std::is_nothrow_default_constructible_v<list>) : container_base_dbg() {}; // no work to do
        container_base_dbg(container_base_dbg&& o) noexcept(noexcept(std::declval<list&>() = std::move(std::declval<list&>())));
        void release_proxy() noexcept;
        void lock() noexcept;
        void unlock() noexcept;
        void swap_proxies(container_base_dbg& o);
        void move_proxies(container_base_dbg& o);

        list list_;
        multithreading::spin_lock sl_;
        std::atomic<uint32_t> ver_;
      };

      //TODO: правильно визначити конструктори та оператори
      class iterator_base_dbg {
      public:
        iterator_base_dbg(const iterator_base_dbg& o);
        iterator_base_dbg(iterator_base_dbg&& o);
        iterator_base_dbg(container_base_dbg* container);
        iterator_base_dbg(const container_base_dbg* container);
        void release();
        void copy_proxy(const iterator_base_dbg& o);

        const container_base_dbg* container_ = nullptr;
        container_base_dbg::list::node_type* proxy_ = nullptr;
      private:
        void register_to_container(const container_base_dbg* container);
      };

      class container_base_rls;
      class iterator_base_rls {
      public:
        iterator_base_rls(const iterator_base_rls&) noexcept {};
        iterator_base_rls(iterator_base_rls&&) noexcept {};
        iterator_base_rls(container_base_rls*) noexcept {};
        iterator_base_rls(const container_base_rls*) noexcept {};
        FORCE_INLINE void release() noexcept {};
        FORCE_INLINE void copy_proxy(const iterator_base_rls&) noexcept {};
      private:
        FORCE_INLINE void register_to_container(const container_base_rls*) noexcept {};
      };

      class container_base_rls {
      public:
        container_base_rls() noexcept {};
        container_base_rls(const container_base_rls&) noexcept {}; // no work to do
        container_base_rls(container_base_rls&&) noexcept {};
        FORCE_INLINE void release_proxy() noexcept {};
        FORCE_INLINE void lock() noexcept {};
        FORCE_INLINE void unlock() noexcept {};
        FORCE_INLINE void swap_proxies(container_base_dbg&) noexcept {};
        FORCE_INLINE void move_proxies(container_base_dbg&) noexcept {};
      };
    }
#ifdef DEBUG_ITERATORS
    using container_base = debug::container_base_dbg;
    using iterator_base = debug::iterator_base_dbg;
#else
    using container_base = debug::container_base_rls;
    using iterator_base = debug::iterator_base_rls;
#endif // DEBUG

  }
}