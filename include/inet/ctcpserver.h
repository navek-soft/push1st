#pragma once
#include <unistd.h>

#include <memory>

#include "cacceptor.h"
#include "cserver.h"
#include "log/clog.h"

namespace inet {
class ctcpserver : public inet::cserver, public cacceptor {
    log_as(tcpserver);

   public:
    ctcpserver(const std::string& nameServer, size_t numthreads, size_t numaccept, bool enableClientNonBlockingMode, std::time_t timeoutMsRecv = 1500, std::time_t timeoutMsSend = 2000, bool enableKeepAlive = false, int KeepAliveRetries = 3, int KeepAliveTimeout = 2, int KeepAliveInterval = 2);
    ~ctcpserver() override;

   protected:
    void TcpListen(const std::string_view& listenHostPort, size_t maxlisten, int af, const ssl_ctx_t& sslContext);
    void TcpJoin();

    virtual ssize_t OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
    virtual std::shared_ptr<inet::ctcpserver> TcpSelf() = 0;
    inline std::shared_ptr<cserver> GetInstance() override {
        return std::dynamic_pointer_cast<cserver>(TcpSelf());
    }

    inline inet::poll_t& PeekPoll() {
        if (svcPolls.empty()) {
            throw std::invalid_argument("service IO is not running");
        }
        auto seq = svcIoSeq++;
        if (svcIoSeq == std::numeric_limits<size_t>::max()) {
            svcIoSeq = 0;
        }
        return svcPolls.at(seq % svcPolls.size());
    }

   private:
    ssize_t OnAccept(fd_t fd, uint events) override;
    ssize_t Accept(fd_t, std::weak_ptr<cpoll> poll) override;
    ssize_t AcceptSSL(fd_t, std::weak_ptr<cpoll> poll) override;

   private:
    bool cliNonBlock {false};
    std::vector<poll_t> svcPolls {};
    mutable std::atomic_uint64_t svcIoSeq {0};
    std::time_t DefaultRecvTimeout {1500}, DefaultSendTimeout {1500};
    bool DefaultKeepAliveEnable {false};
    int DefaultKeepAliveRetries {3}, DefaultKeepAliveTimeout {2}, DefaultKeepAliveInterval {2};
    ssl_ctx_t srvSslContext;
};
}// namespace inet
