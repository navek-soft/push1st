#pragma once
#include <atomic>
#include <mutex>

namespace core {
class cspinlock : protected std::atomic_flag {
   public:
    cspinlock() noexcept : atomic_flag ATOMIC_FLAG_INIT {
        ;
    }
    inline void lock() {                                 // NOLINT ( : protected std::atomic_flag)
        while (test_and_set(std::memory_order_acquire)) {// acquire lock
#if defined(__cpp_lib_atomic_flag_test)
            while (test(std::memory_order_relaxed)) {
                ;
            }// test lock
#endif
            ;
        }
    }
    inline bool try_lock() {// NOLINT ( : protected std::atomic_flag)
        return test_and_set(std::memory_order_acquire);
    }
    inline void unlock() {// NOLINT ( : protected std::atomic_flag)
        clear(std::memory_order_release);
        ;
    }
};
}// namespace core

using spinlock_t = core::cspinlock;