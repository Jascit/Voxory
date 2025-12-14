#pragma once
#include <platform/platform.h>
#include <utility.h>
#include <multithreading/spin_lock.h>
#include <mutex>
#include <containers/impl/list.h>

namespace voxory {
  namespace containers {
    namespace debug {
      //TODO: правильно визначити конструктори та оператори
      class container_base_dbg {
      public:
        container_base_dbg& operator=(container_base_dbg&& o);
        void release_proxy() noexcept;
        void swap_proxies(container_base_dbg& o);
        void move_proxies(container_base_dbg& o);
        void lock();
        void unlock();

        list list_;
        multithreading::spin_lock sl_;
        std::atomic<uint32_t> ver_;
      };

      class iterator_base_dbg {
      public:
        const container_base_dbg* container_ = nullptr;
        node* proxy_ = nullptr;
        iterator_base_dbg(const iterator_base_dbg& o);
        iterator_base_dbg(iterator_base_dbg&& o) {};
        iterator_base_dbg(container_base_dbg* container);
        iterator_base_dbg(const container_base_dbg* container);
        iterator_base_dbg& operator=(const iterator_base_dbg& o) { return *this; };
        iterator_base_dbg& operator=(iterator_base_dbg&& o);
        void release_on_destroy();
      private:
        void register_to_container(const container_base_dbg* container);
      };

      class iterator_base_rls {
      public:
        iterator_base_rls(container_base_dbg* container) noexcept {};
        iterator_base_rls(const container_base_dbg* container) noexcept {};
        FORCE_INLINE void release_on_destroy() noexcept {};
      };

      class container_base_rls {
      public:
        FORCE_INLINE void move_proxies(container_base_dbg&) noexcept {}
        FORCE_INLINE void swap_proxies(container_base_dbg&) noexcept {}
        FORCE_INLINE void release_proxy() noexcept {}
        FORCE_INLINE void lock() noexcept {}
        FORCE_INLINE void unlock() noexcept {}
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