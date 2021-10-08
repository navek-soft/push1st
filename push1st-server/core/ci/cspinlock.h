#pragma once
#include <mutex>
#include <atomic>

namespace core {
	class cspinlock : protected std::atomic_flag {
	public:
		cspinlock() noexcept : atomic_flag{ ATOMIC_FLAG_INIT } { ; }
		inline void lock() {
			while (test_and_set(std::memory_order_acquire)) {  // acquire lock
#if defined(__cpp_lib_atomic_flag_test)
				while (lock.test(std::memory_order_relaxed))        // test lock
#endif
					;
			}
		}
		inline bool try_lock() { return test_and_set(std::memory_order_acquire); }
		inline void unlock() { clear(std::memory_order_release); ; }
	};
}

using spinlock_t = core::cspinlock;