#include "inet/ctcpserver.h"

#include <unistd.h>

#include <csignal>
#include <memory>

#include "inet/cepoll.h"
#include "inet/cserver.h"
#include "log/clog.h"

using namespace inet;

ssize_t ctcpserver::OnAccept(fd_t fd, uint events) {
    PSHT_DEBUG("on accept {} events {}", fd, events);
    if (events == EPOLLIN) {
        auto&& poll = PeekPoll();
        return srvSslContext ? AcceptSSL(fd, poll) : Accept(fd, poll);
    }

    PSHT_ERROR("server {} poll error: {}{}{}{}", fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
    inet::Close(fd);
    raise(SIGPIPE);

    return -EINVAL;
}

ssize_t ctcpserver::Accept(fd_t fd, std::weak_ptr<cpoll> poll) {
    sockaddr_storage sa;
    ssize_t res {-1};
    int cli {-1};
    if (res = inet::TcpAccept(fd, cli, sa, cliNonBlock); res == 0) {
        inet::SetRecvTimeout(cli, DefaultRecvTimeout);
        inet::SetSendTimeout(cli, DefaultSendTimeout);
        inet::SetKeepAlive(cli, DefaultKeepAliveEnable, DefaultKeepAliveRetries, DefaultKeepAliveTimeout, DefaultKeepAliveInterval);
        if ((res = OnTcpAccept(cli, sa, {}, poll)) == 0) {
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

ssize_t ctcpserver::AcceptSSL(fd_t fd, std::weak_ptr<cpoll> poll) {
    sockaddr_storage sa;
    ssize_t res {-1};
    int cli {-1};
    ssl_t cliSSL;
    if ((res = inet::TcpAccept(fd, cli, sa, cliNonBlock)) == 0 and (res = inet::SslAcceptConnection(cli, srvSslContext, cliSSL)) == 0) {
        inet::SetRecvTimeout(cli, DefaultRecvTimeout);
        inet::SetSendTimeout(cli, DefaultSendTimeout);
        inet::SetKeepAlive(cli, DefaultKeepAliveEnable, DefaultKeepAliveRetries, DefaultKeepAliveTimeout, DefaultKeepAliveInterval);

        if ((res = OnTcpAccept(cli, sa, cliSSL, poll)) == 0) {
            return 0;
        }

        PSHT_ERROR("connection {} error: {}", cli, inet::GetErrorStr(res));
    } else {
        PSHT_ERROR("accept {} error: {}", fd, inet::GetErrorStr(res));
    }

    inet::Close(cli);
    return res;
}

void ctcpserver::TcpListen(const std::string_view& listenHostPort, size_t maxlisten, int af, const ssl_ctx_t& sslContext) {
    for (auto&& poll : svcPolls) {
        poll->Listen();
    }
    srvSslContext = sslContext;
    cacceptor::Listen(listen_t {.proto = IPPROTO_TCP, .maxlisten = maxlisten, .af = af, .hostport = listenHostPort});
}

void ctcpserver::TcpJoin() {
    cacceptor::Join();
    for (const auto& poll : svcPolls) {
        poll->Join();
    }
}

ctcpserver::ctcpserver(const std::string& nameServer, size_t numthreads, size_t numaccept, bool enableClientNonBlockingMode, std::time_t timeoutMsRecv, std::time_t timeoutMsSend, bool enableKeepAlive, int KeepAliveRetries, int KeepAliveTimeout, int KeepAliveInterval) :
    cserver {nameServer},
    cacceptor(nameServer, numaccept),
    cliNonBlock {enableClientNonBlockingMode},
    DefaultRecvTimeout {timeoutMsRecv},
    DefaultSendTimeout {timeoutMsSend},
    DefaultKeepAliveEnable {enableKeepAlive},
    DefaultKeepAliveRetries {KeepAliveRetries},
    DefaultKeepAliveTimeout {KeepAliveTimeout},
    DefaultKeepAliveInterval {KeepAliveInterval} {
    for (size_t i = 0; i < numthreads; ++i) {
        svcPolls.emplace_back(std::make_shared<inet::cpoll>(fmt::format("{}-worker-{}", nameServer, i)));
    }
}

ctcpserver::~ctcpserver() {}
