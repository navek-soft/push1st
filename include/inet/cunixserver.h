#pragma once
#include "cserver.h"
#include "log/clog.h"

namespace inet {
class cunixserver : public inet::cserver {
    log_as(unixserver);

   public:
    static inline size_t DefaultRecvTimeout {1500}, DefaultSendTimeout {1500};

    cunixserver(const std::string& nameServer, bool enableClientNonBlockingMode, std::time_t clientActivityTimeout);
    ~cunixserver() override;
    void Listen(const std::shared_ptr<cpoll>& poll) override;

   protected:
    virtual std::shared_ptr<inet::cunixserver> UnixSelf() = 0;
    inline std::shared_ptr<cserver> GetInstance() override {
        return std::dynamic_pointer_cast<cserver>(UnixSelf());
    }
    virtual ssize_t OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
    ssize_t UnixListen(const std::string_view& listenFilePath, bool nonblock, int maxlisten);
    void UnixClose();

   private:
    void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) override;
    inline void OnAcceptSSL(fd_t, uint, [[maybe_unused]] std::weak_ptr<cpoll> poll) override {
        ;
    }
    fd_t srvFd {-1};
    bool cliNonBlock {false};
};
}// namespace inet

using serverunix_t = inet::cunixserver;