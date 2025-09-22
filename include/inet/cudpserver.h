#pragma once
#include "cserver.h"
#include "csocket.h"
#include "log/clog.h"

namespace inet {
class cudpserver : public inet::cserver {
    log_as(udpserver);

   public:
    cudpserver(const std::string& nameServer);
    ~cudpserver() override;
    void Listen(const std::shared_ptr<cpoll>& poll);

   protected:
    inline inet::csocket Fd() {// NOLINT
        return {srvFd, nullptr};
    }
    ssize_t UdpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock);
    void UdpClose();
    virtual ssize_t OnUdpData(fd_t fd, const inet::ssl_t& ssl) = 0;
    virtual std::shared_ptr<inet::cudpserver> UdpSelf() = 0;
    inline std::shared_ptr<cserver> GetInstance() override {
        return std::dynamic_pointer_cast<cserver>(UdpSelf());
    }

   private:
    ssize_t OnAccept(fd_t fd, uint events);
    ssize_t Accept(fd_t, std::weak_ptr<cpoll> /*poll*/) override;
    inline ssize_t AcceptSSL(fd_t, std::weak_ptr<cpoll> /*poll*/) override {
        return 0;
    }

   private:
    fd_t srvFd {-1};
};
}// namespace inet
