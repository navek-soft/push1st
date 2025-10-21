#pragma once
#include <sys/epoll.h>

#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "cinet.h"
#include "log/clog.h"

namespace inet {
class cpoll : public std::enable_shared_from_this<cpoll> {
    log_as(poll);

   public:
    class cgc {
       public:
        virtual ~cgc() = default;
        virtual inline bool IsLeaveUs(std::time_t now) = 0;
    };

    cpoll(const std::string& name);
    virtual ~cpoll();
    virtual const char* NameOf() {
        return name.empty() ? "epoll" : name.c_str();
    }
    ssize_t Listen(int pollSize = 1024, int pollTimeoutMs = -1);
    void Join();

   protected:
    inline std::shared_ptr<cpoll> Self() {
        return shared_from_this();
    }

   public:
    template <typename OBJ>
    inline void EnqueueGc(const std::shared_ptr<OBJ>& obj);
    template <typename FN>
    inline ssize_t PollAdd(fd_t fd, uint events, FN&& callback);
    template <typename FN>
    inline ssize_t PollUpdate(fd_t fd, FN&& callback);
    template <typename FN>
    inline ssize_t PollUpdate(fd_t fd, uint events, FN&& callback);
    ssize_t PollUpdate(fd_t fd, uint events);
    static void PollDelete(int& fd);
    inline fd_t Fd() const {
        return fdPoll;
    }

   private:
    inline void Gc();
    static void PollThread(const std::shared_ptr<cpoll>& self, int numEventsMax, [[maybe_unused]] int msTimeout);
    using handler_t = std::function<void(fd_t, uint)>;
    std::string name;
    fd_t fdPoll {-1}, exitFd {-1};
    static inline std::unordered_map<fd_t, handler_t> fdHandlers;
    static inline std::mutex fdLock;
    std::list<std::weak_ptr<cgc>> fdQueueGC;
    std::jthread pollThread;
};

template <typename OBJ>
inline void cpoll::EnqueueGc(const std::shared_ptr<OBJ>& obj) {
    std::unique_lock<decltype(fdLock)> lock(fdLock);
    fdQueueGC.emplace_back(std::dynamic_pointer_cast<cgc>(obj));
}

template <typename FN_T>
inline ssize_t cpoll::PollAdd(fd_t fd, uint events, FN_T&& callback) {
    std::unique_lock<decltype(fdLock)> lock(fdLock);
    if (struct epoll_event ev {.events = events, .data = {.fd = fd}}; epoll_ctl(fdPoll, EPOLL_CTL_ADD, fd, &ev) == 0) {
        fdHandlers[fd] = std::forward<FN_T>(callback);
        return 0;
    }
    return -errno;
}
template <class FN_T>
inline ssize_t cpoll::PollUpdate(fd_t fd, FN_T&& callback) {
    std::unique_lock<decltype(fdLock)> lock(fdLock);
    if (auto&& it {fdHandlers.find(fd)}; it != fdHandlers.end()) {
        fdHandlers[fd] = std::forward<FN_T>(callback);
        return 0;
    }
    return -ENOENT;
}
template <class FN_T>
inline ssize_t cpoll::PollUpdate(fd_t fd, uint events, FN_T&& callback) {
    std::unique_lock<decltype(fdLock)> lock(fdLock);
    if (auto&& it {fdHandlers.find(fd)}; it != fdHandlers.end()) {
        fdHandlers[fd] = std::forward<FN_T>(callback);
        return PollUpdate(fd, events);
    }
    return -ENOENT;
}

using poll_t = std::shared_ptr<cpoll>;
}// namespace inet
