#pragma once 
#include <atomic>
#include <platform/platform.h>

namespace voxory {
  namespace multithreading {
    class spin_lock {
    public:
      spin_lock() noexcept : flag_(ATOMIC_FLAG_INIT) {};
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