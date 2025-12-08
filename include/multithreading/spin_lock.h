#pragma once 
#include <atomic>
#include <platform/platform.h>

namespace voxory {
  namespace multithreading {
    class spin_lock {
    public:
      spin_lock() noexcept {}

      // Заборонити копіювання
      spin_lock(const spin_lock&) = delete;
      spin_lock& operator=(const spin_lock&) = delete;

      // Дозволити переміщення (не робить реального переміщення, але потрібно для контейнерів)
      spin_lock(spin_lock&&) noexcept {}
      spin_lock& operator=(spin_lock&&) noexcept { return *this; }

      ~spin_lock() noexcept = default;

      void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
          mm_pause(); // якщо у тебе є архітектурна пауза
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