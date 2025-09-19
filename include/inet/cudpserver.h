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
    void Listen(const std::shared_ptr<cpoll>& poll) override;

   protected:
    inline inet::csocket Fd() {// NOLINT
        return {srvFd, nullptr};
    }
    ssize_t UdpListen(const std::string_view& iface, const std::string_view& port, bool reuseaddress, bool reuseport, bool nonblock);
    ssize_t UdpListen(const std::string_view& listenHostPort, bool reuseaddress, bool reuseport, bool nonblock);
    void UdpClose();
    virtual void OnUdpData(fd_t fd, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
    virtual std::shared_ptr<inet::cudpserver> UdpSelf() = 0;
    inline std::shared_ptr<cserver> GetInstance() override {
        return std::dynamic_pointer_cast<cserver>(UdpSelf());
    }

   private:
    void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) override;
    inline void OnAcceptSSL(fd_t, uint, [[maybe_unused]] std::weak_ptr<cpoll> poll) override {
        ;
    }

   private:
    fd_t srvFd {-1};
};
}// namespace inet
