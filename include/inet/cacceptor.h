#pragma once
#include <core/util/cfactory.h>
#include <core/util/cspinlock.h>
#include <inet/ciface.h>
#include <log/clog.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "cepoll.h"
#include "cinet.h"

namespace inet {

const static std::set<ssize_t> ignore_errors {EAGAIN, ENETDOWN, EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOTSUPP, ENETUNREACH};

class cacceptor {
    log_as(accept);

   public:
    struct listen_t {
        int proto;
        size_t maxlisten;
        int af;
        std::string_view hostport;
    };

   public:
    cacceptor(const std::string& name, size_t numthreads) : name {name}, numworkers {numthreads} {
        serverPolls.reserve(numworkers);
        for (auto n {numworkers}; n--;) {
            serverPolls.emplace_back(std::make_shared<inet::cpoll>());
        }
    }

    cacceptor(cacceptor&& fd) = delete;
    cacceptor& operator=(cacceptor&& fd) = delete;
    cacceptor(const cacceptor& fd) = delete;
    cacceptor& operator=(const cacceptor& fd) = delete;
    ~cacceptor() = default;

   public:
    void Listen(const listen_t& listen) {
        for (size_t i = 0; i < numworkers; ++i) {
            auto newFd = MakeListen(listen);
            auto& epoll = serverPolls.at(i);

            if (auto&& res = epoll->PollAdd(newFd,
                                            EPOLLIN | EPOLLEXCLUSIVE,
                                            [this, poll = epoll->weak_from_this()](auto fd, auto events) {
                                                if (auto&& self = poll.lock(); self) {
                                                    if (auto&& res = OnAccept(fd, events); res < 0 and not ignore_errors.contains(-res)) {
                                                        inet::Close(fd);
                                                        return;
                                                    }
                                                }
                                            });
                res < 0) {
                throw std::runtime_error(fmt::format("[{}] error {}:{}", name, __FUNCTION__, std::strerror(-res)));
            }

            epoll->Listen();
        }
    }

   protected:
    void Join() {
        for (auto& poll : serverPolls) {
            poll->Join();
        }
    }
    virtual ssize_t OnAccept(fd_t fd, uint events) = 0;

   private:
    fd_t MakeListen(const listen_t& listen) {
        fd_t srvFd {-1};
        sockaddr_storage sa;

        if (auto res = inet::GetSockAddr(sa, listen.hostport, "0", listen.af); res < 0) {
            throw std::runtime_error(fmt::format("[{}] can not get address {}:{}", name, -res, std::strerror(-res)));
        }

        switch (listen.proto) {
            case IPPROTO_TCP: {
                if (auto&& res = inet::TcpServer(srvFd, sa, true, true, false, listen.maxlisten); res < 0) {
                    throw std::runtime_error(fmt::format("[{}] create listen TCP socket {}:{}", name, -res, std::strerror(-res)));
                }
                break;
            }
            case IPPROTO_UDP: {
                if (auto&& res = inet::UdpServer(srvFd, sa, true, true, false); res < 0) {
                    throw std::runtime_error(fmt::format("[{}] create listen UDP socket {}:{}", name, -res, std::strerror(-res)));
                }
                break;
            }
            case IPPROTO_UNIX: {
                if (auto&& res = inet::UnixServer(srvFd, sa, false, listen.maxlisten); res < 0) {
                    throw std::runtime_error(fmt::format("[{}] create listen UNIX socket {}:{}", name, -res, std::strerror(-res)));
                }
                break;
            }
            default:
                throw std::runtime_error(fmt::format("[{}] wrong protocol type {}", name, listen.proto));
        };

        PSHT_INFO("[{}] listening {} socket {} on `{}`",
                  name,
                  (listen.proto == IPPROTO_TCP)   ? "TCP"
                  : (listen.proto == IPPROTO_UDP) ? "UDP"
                                                  : "UNIX",
                  srvFd,
                  listen.hostport);
        svcSo.emplace(srvFd);
        return srvFd;
    }

   private:
    const std::string name;
    std::vector<std::shared_ptr<inet::cpoll>> serverPolls;
    std::unordered_set<fd_t> svcSo {};
    const size_t numworkers {0};
};

using listen_t = cacceptor::listen_t;

}// namespace inet
