#include <core/util/ctimer.h>
#include <inet/cepoll.h>

#include "log/clog.h"

namespace core {
static inline void SetTimer(int fd, unsigned int initial_ms, unsigned int interval_ms) {
    struct itimerspec newValue {};
    // convert milliseconds to seconds and nanoseconds
    newValue.it_value.tv_sec = initial_ms / 1000;
    newValue.it_value.tv_nsec = (initial_ms % 1000) * 1000000;

    if (interval_ms > 0) {
        newValue.it_interval.tv_sec = interval_ms / 1000;
        newValue.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;
    }

    if (timerfd_settime(fd, 0, &newValue, nullptr)) {
        throw std::runtime_error(fmt::format("Failed to set timerfd {}", fd));
    }
}

template <timer_type_t Type>
bool ctimer<Type>::Launch(unsigned int ms, std::function<bool(std::time_t)>&& fn) {
    std::lock_guard _ {timerLock};
    if (running) {
        return false;
    }
    func = std::move(fn);

    Insert();

    unsigned int interval = 0;
    if constexpr (Type == timer_type_t::periodic) {
        interval = ms;
    }

    SetTimer(Fd(), ms, interval);
    running = true;
    return true;
}

template <timer_type_t Type>
void ctimer<Type>::TimerOnEvent([[maybe_unused]] fd_t fd, uint events) {
    auto now = std::time(nullptr);

    if (events == EPOLLIN) {
        if (int fd = timerFd.load(); fd > 0) {
            uint64_t expirations {0};
            if (auto res {::read(fd, &expirations, sizeof(expirations))}; res < 0) {
                if (res = errno; res == EAGAIN) {
                    PSHT_WARNING("Timer {} read {} {}", fd, res, std::strerror(res));
                    Insert();
                    return;
                }
                PSHT_ERROR("Timer {} read {} {}", fd, res, std::strerror(res));
                return;
            }
            if constexpr (Type == timer_type_t::oneshot) {
                StopTimer();
            }

            if (not func(now)) {
                if constexpr (Type == timer_type_t::periodic) {
                    Join();
                }
                return;
            }

            if constexpr (Type == timer_type_t::periodic) {
                Insert();
            }
        }
    }
}

template <timer_type_t Type>
bool ctimer<Type>::StopTimer(const std::unique_lock<cspinlock>&) {
    if (not running) {
        return false;
    }

    running = false;

    struct itimerspec zero {};
    zero.it_value.tv_sec = 0;
    zero.it_value.tv_nsec = 0;
    zero.it_interval.tv_sec = 0;
    zero.it_interval.tv_nsec = 0;
    if (timerfd_settime(timerFd, 0, &zero, nullptr)) {
        throw std::runtime_error("Failed to stop timer");
    }

    return true;
}

template <timer_type_t Type>
bool ctimer<Type>::StopTimer() {
    std::unique_lock lock {timerLock};
    return StopTimer(lock);
}

template <timer_type_t Type>
void ctimer<Type>::Join() {
    std::unique_lock lock {timerLock};
    StopTimer(lock);
    if (int fd {timerFd.exchange(-1)}; fd > 0) {
        if (auto&& poll {fdPoll.lock()}; poll) {
            poll->PollDelete(fd);
        }
        ::close(fd);
    }
}

template <timer_type_t Type>
int ctimer<Type>::Fd() const {
    return timerFd;
}

template <timer_type_t Type>
ssize_t ctimer<Type>::Insert() {
    auto&& poll {fdPoll.lock()};
    if (not poll) {
        return -EPERM;
    }

    if (int fd = timerFd.load(); fd < 0) {
        if (timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); timerFd == -1) {
            throw std::runtime_error("Failed to create timer");
        }
    }

    auto&& callback = [weak = ctimer<Type>::weak_from_this()](auto events, auto now) {
        if (auto self = weak.lock(); self) {
            self->TimerOnEvent(events, now);
        }
    };

    if (auto&& res = poll->PollAdd(timerFd.load(), EPOLLIN, callback); res < 0) {
        PSHT_ERROR("Timer {} {}:{}", Fd(), -res, std::strerror(-res));
        return res;
    }

    return 0;
}

template <timer_type_t Type>
ctimer<Type>::ctimer(const std::shared_ptr<inet::cpoll>& fdPoll) : fdPoll {fdPoll} {}

template <timer_type_t Type>
ctimer<Type>::~ctimer() {
    if (int fd {-1}; (fd = timerFd.exchange(-1)) > 0) {
        if (auto&& poll {fdPoll.lock()}; poll) {
            poll->PollDelete(fd);
        }
        ::close(fd);
    }
}

template class ctimer<timer_type_t::oneshot>;
template class ctimer<timer_type_t::periodic>;

}// namespace core