#include "inet/cunixserver.h"

#include <csignal>

#include "inet/cepoll.h"
#include "log/clog.h"

using namespace inet;

void cunixserver::OnAccept(fd_t fd, uint events, std::weak_ptr<cpoll> poll) {
    if (events == EPOLLIN) {
        sockaddr_storage sa;
        ssize_t res {-1};
        fd_t cli {-1};
        if (res = inet::UnixAccept(fd, cli, sa, cliNonBlock); res == 0) {
            if ((res = OnUnixAccept(cli, sa, {}, poll)) == 0) {
                return;
            }
            PSHT_ERROR("connection {} error: {}", cli, inet::GetErrorStr(res));
            inet::Close(cli);
        } else if (res != -EAGAIN) {
            PSHT_ERROR("accept {} error: {}", fd, inet::GetErrorStr(res));
        }
        return;
    }
    PSHT_ERROR("server {} poll error: {}{}{}{}", fd, (events & EPOLLIN) ? "EPOLLIN " : "", (events & EPOLLRDHUP) ? "EPOLLRDHUP " : "", (events & EPOLLHUP) ? "EPOLLHUP " : "", (events & EPOLLERR) ? "EPOLLERR " : "");
    inet::Close(fd);
    raise(SIGPIPE);
}

void cunixserver::Listen(const std::shared_ptr<cpoll>& poll) {
    if (srvFd > 0) {
        ssize_t res {-1};
        if (res = poll->PollAdd(srvFd,
                                EPOLLIN | EPOLLEXCLUSIVE,
                                [this, capture0 = poll->weak_from_this()](auto&& PH1, auto&& PH2) {
                                    OnAccept(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), capture0);
                                });
            res == 0) {
            return;
        }
        PSHT_ERROR("Listen error: {}", std::strerror((int)-res));
        throw std::runtime_error("UNIX server fail");
    }
}

ssize_t cunixserver::UnixListen(const std::string_view& listenFilePath, bool nonblock, int maxlisten) {
    ssize_t res {-1};
    if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, listenFilePath, "0", AF_UNIX)) == 0 and (res = inet::UnixServer(srvFd, sa, nonblock, maxlisten)) == 0) {
        return res;
    }
    inet::Close(srvFd);
    return res;
}

void cunixserver::UnixClose() {
    inet::Close(srvFd);
}

cunixserver::cunixserver(const std::string& nameServer, bool enableClientNonBlockingMode, [[maybe_unused]] std::time_t clientActivityTimeout) :
    cserver {nameServer},
    cliNonBlock {enableClientNonBlockingMode} {}

cunixserver::~cunixserver() {
    UnixClose();
}