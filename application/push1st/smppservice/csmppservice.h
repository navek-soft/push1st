#pragma once
#include <atomic>
#include <mutex>

#include "core/hooks/chooks.h"
#include "log/clog.h"

/*
 * https://github.com/onlinecity/cpp-smpp/
 */

class csmppservice : public std::enable_shared_from_this<csmppservice> {
    log_as(smppservice);

    class cgateway : public inet::cpoll::cgc, public std::enable_shared_from_this<cgateway> {
       public:
        cgateway(const std::shared_ptr<csmppservice>& svc, const std::shared_ptr<inet::cpoll>& poll, const std::shared_ptr<cwebhook>& hook, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);
        ~cgateway() override;

        inline bool Connect();
        ssize_t Send(const std::string& msg, std::string& response);
        ssize_t Send(const std::string& msg);
        void Assign(const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port);

        inline uint32_t Seq() {
            return ++seqNo;
        }
        inline uint32_t MsgId() {
            return seqNo;
        }
        inline const std::string& Channel() {
            return gwConId;
        }
        inline int Fd() const {
            return gwSocket.Fd();
        }
        inline void Close() {
            gwSocket.SocketClose();
        }
        bool IsLeaveUs(std::time_t now) override;

       private:
        void OnGwReply(fd_t, uint);
        inline void OnDeliveryStatus(const std::string& data);

       private:
        std::shared_ptr<csmppservice> svc;
        std::string gwLogin, gwPassword;
        std::vector<sockaddr_storage> gwHosts;
        mutable std::mutex gwSocketLock;
        inet::csocket gwSocket;
        mutable std::atomic_uint32_t seqNo {0};
        size_t gwHash {0};
        std::string gwConId;
        std::shared_ptr<inet::cpoll> gwPoll;
        std::shared_ptr<cwebhook> gwHook;
        std::time_t gwPingTime {0}, gwPingInterval {30};
    };

   public:
    csmppservice(const std::string& webhook = {});
    virtual ~csmppservice() {
        gwPoll->Join();
    };
    inline void Listen() {
        gwPoll->Listen();
    }
    std::pair<std::string, json::value_t> Send(const json::value_t& message);
    void ReleaseGateway(const std::string& gwLogin, const std::string& gwPassword);

   private:
    std::mutex gwConnectionLock;
    std::unordered_map<std::string, std::shared_ptr<cgateway>> gwConnections;
    std::shared_ptr<inet::cpoll> gwPoll;
    std::shared_ptr<cwebhook> gwHook;
};