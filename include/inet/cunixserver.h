#pragma once

#include "cacceptor.h"
#include "cserver.h"
#include "log/clog.h"

namespace inet {
class cunixserver : public inet::cserver, public cacceptor {
    log_as(unixserver);

   public:
    static inline size_t DefaultRecvTimeout {1500}, DefaultSendTimeout {1500};

    cunixserver(const std::string& nameServer, size_t numthreads, size_t numaccept, bool enableClientNonBlockingMode, std::time_t clientActivityTimeout);
    ~cunixserver() override;

   protected:
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
    virtual std::shared_ptr<inet::cunixserver> UnixSelf() = 0;
    inline std::shared_ptr<cserver> GetInstance() override {
        return std::dynamic_pointer_cast<cserver>(UnixSelf());
    }
    virtual ssize_t OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) = 0;
    void UnixListen(const std::string_view& listenHostPort, size_t maxlisten, int af);
    void UnixJoin();

   private:
    ssize_t OnAccept(fd_t fd, uint events) override;

    ssize_t Accept(fd_t, std::weak_ptr<cpoll>) override;
    inline ssize_t AcceptSSL(fd_t, std::weak_ptr<cpoll>) override {
        return 0;
    }

   private:
    std::vector<poll_t> svcPolls {};
    mutable std::atomic_uint64_t svcIoSeq {0};
    bool cliNonBlock {false};
};
}// namespace inet

using serverunix_t = inet::cunixserver;