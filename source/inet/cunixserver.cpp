#include "inet/cunixserver.h"

#include <csignal>

#include "inet/cacceptor.h"
#include "inet/cepoll.h"
#include "log/clog.h"

namespace inet {
ssize_t cunixserver::OnAccept(fd_t fd, uint events) {
    if (events == EPOLLIN) {
        auto&& poll = PeekPoll();
        return Accept(fd, poll);
    }
    PSHT_ERROR("server {} poll error: {}{}{}{}", fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
    inet::Close(fd);
    raise(SIGPIPE);

    return -EINVAL;
}

ssize_t cunixserver::Accept(fd_t fd, std::weak_ptr<cpoll> poll) {
    sockaddr_storage sa;
    ssize_t res {-1};
    fd_t cli {-1};
    if (res = inet::UnixAccept(fd, cli, sa, cliNonBlock); res == 0) {
        if ((res = OnUnixAccept(cli, sa, {}, poll)) == 0) {
            return 0;
        }
        PSHT_ERROR("connection {} error: {}", cli, inet::GetErrorStr(res));
        inet::Close(cli);
    } else {
        PSHT_ERROR("accept {} error: {}", fd, inet::GetErrorStr(res));
        if (res != -EAGAIN) {
            inet::Close(cli);
        }
    }

    return res;
}

void cunixserver::UnixListen(const std::string_view& listenHostPort, size_t maxlisten, int af) {
    for (auto&& poll : svcPolls) {
        poll->Listen();
    }
    cacceptor::Listen(listen_t {.proto = IPPROTO_UNIX, .maxlisten = maxlisten, .af = af, .hostport = listenHostPort});
}

void cunixserver::UnixJoin() {
    cacceptor::Join();
    for (const auto& poll : svcPolls) {
        poll->Join();
    }
}

cunixserver::cunixserver(const std::string& nameServer, size_t numthreads, size_t numaccept, bool enableClientNonBlockingMode, [[maybe_unused]] std::time_t clientActivityTimeout) :
    cserver {nameServer},
    cacceptor(nameServer, numaccept),
    cliNonBlock {enableClientNonBlockingMode} {
    for (size_t i = 0; i < numthreads; ++i) {
        svcPolls.emplace_back(std::make_shared<inet::cpoll>(fmt::format("{}-worker-{}", nameServer, i)));
    }
}

cunixserver::~cunixserver() {}
}// namespace inet
