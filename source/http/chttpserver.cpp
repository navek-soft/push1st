#include "http/chttpserver.h"

#include <memory>

using namespace inet;

ssize_t chttpserver::OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
    ssize_t res {-1};
    if (auto&& self {poll.lock()}; self) {
        return self->PollAdd(fd, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR, [this, sa, ssl, poll](auto&& PH1, auto&& PH2) {
            HttpData(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), sa, ssl, poll);
        });
    }
    return res;
}

void chttpserver::HttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
    ssize_t res {-1};
    if (auto&& self {poll.lock()}; self) {
        if (auto&& so = std::make_shared<inet::csocket>(fd, sa, ssl, poll); events == EPOLLIN) {
            std::string_view method, content;
            http::uri_t path;
            http::headers_t headers;
            std::string request;
            if (res = HttpReadRequest(*so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
                OnHttpRequest(so, method, path, headers, request, content);
            } else {
                OnHttpError(so, res);
            }
        } else {
            so->SocketClose();
        }
        return;
    }
    inet::Close(fd);
}

chttpserver::chttpserver(const std::string& name, size_t numthreads, size_t numaccept, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize) :
    inet::ctcpserver {name, numthreads, numaccept, false, 1500, 2000, true},
    HttpMaxHeaderSize {httpMaxHeaderSize} {
    TcpListen(HostPort, 4096, AF_INET, SslCtx);
}

chttpserver::~chttpserver() {}
