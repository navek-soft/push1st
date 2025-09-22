#pragma once
#include <inet/ctcpserver.h>

#include "chttpconn.h"

namespace inet {

class chttpserver : public inet::ctcpserver, public inet::chttpconnection {
   public:
    chttpserver(const std::string& name, size_t numthreads, size_t numaccept, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize = 65536);
    ~chttpserver() override;

   public:
    void Join() {
        inet::ctcpserver::TcpJoin();
    }

   protected:
    virtual void OnHttpRequest(const inet::socket_t& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) = 0;
    virtual void OnHttpError(const inet::socket_t& fd, ssize_t err) = 0;
    inline std::shared_ptr<inet::ctcpserver> TcpSelf() override = 0;

   private:
    ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) override;
    void HttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll);

   private:
    size_t HttpMaxHeaderSize {65536};
};
}// namespace inet