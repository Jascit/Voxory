#pragma once 
#include <atomic>
#include <platform/platform.h>

namespace voxory {
  namespace multithreading {
    class spin_lock {
    public:
      spin_lock() noexcept {}

      spin_lock(const spin_lock&) = delete;
      spin_lock& operator=(const spin_lock& o) {
        if (this == std::addressof(o)) return *this;
        flag_._Storage.store(o.flag_.test(), std::memory_order_seq_cst);
        return *this;
      };

      spin_lock(spin_lock&&) noexcept {}
      spin_lock& operator=(spin_lock&& o) noexcept { 
        if (this == std::addressof(o)) return *this;
        flag_._Storage.store(o.flag_.test(), std::memory_order_seq_cst);
        return *this; 
      }

      ~spin_lock() noexcept = default;

      void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
          mm_pause();
        }
      }

      void unlock() noexcept {
        flag_.clear(std::memory_order_release);
      }

    private:
      std::atomic_flag flag_;
    };
  };
}