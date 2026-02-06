#pragma once

#include <log/clog.h>
#include <sys/timerfd.h>

#include <atomic>
#include <cerrno>
#include <functional>

#include "cfactory.h"
#include "cspinlock.h"

namespace inet {
class cpoll;
}

enum timer_type_t : uint8_t { oneshot = 0, periodic };

namespace core {
template <timer_type_t Type>
class ctimer : public csharedfactory<ctimer<Type>>, public std::enable_shared_from_this<ctimer<Type>> {
    log_as(timer);
    friend class csharedfactory<ctimer<Type>>;

   protected:
    explicit ctimer(const std::shared_ptr<inet::cpoll>& fdPoll);

   public:
    ~ctimer();
    ctimer(const ctimer&) = delete;
    ctimer& operator=(const ctimer&) = delete;

   public:
    void Join();
    int Fd() const;
    bool Launch(unsigned int ms, std::function<bool(std::time_t)>&& fn);
    bool StopTimer();

   private:
    ssize_t Insert();

    void TimerOnEvent(fd_t fd, uint events);

   private:
    bool StopTimer(const std::unique_lock<cspinlock>&);

   private:
    std::weak_ptr<inet::cpoll> fdPoll;
    std::atomic_int timerFd {-1};
    std::function<bool(std::time_t)> func;
    cspinlock timerLock;
    bool running {false};
};

}// namespace core

using oneshot_timer_t = core::ctimer<timer_type_t::oneshot>;
using oneshot_timer_ptr_t = std::shared_ptr<oneshot_timer_t>;

using periodic_timer_t = core::ctimer<timer_type_t::periodic>;
using periodic_timer_ptr_t = std::shared_ptr<periodic_timer_t>;
