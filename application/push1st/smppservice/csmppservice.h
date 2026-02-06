#pragma once
#include <core/hooks/chooks.h>
#include <core/util/casyncqueue.h>
#include <unistd.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>

#include "cratelimiter.h"
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
            svc->ReleaseGateway(gwLogin, gwPassword);
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
        std::time_t gwPingTime {0}, gwPPTimeout {0};
        const std::time_t gwPingInterval {30}, gwTimeoutInterval {60};
    };

   public:
    csmppservice(const std::string& webhook = {}, size_t rps = 100);
    virtual ~csmppservice() {
        gwPoll->Join();
    };

   public:
    inline void Listen() {
        gwPoll->Listen();
    }

    std::pair<std::string, json::value_t> Send(json::value_t&& message);
    void ReleaseGateway(const std::string& gwLogin, const std::string& gwPassword);

   private:
    class cackmsg;

    void SendChunk(const std::shared_ptr<cgateway>& gw, const std::shared_ptr<cackmsg>& chunk);
    std::shared_ptr<cgateway> GetGateway(const std::string& login, const std::string& pass, const std::vector<std::string>& hosts, const std::string& port);

   private:
    std::shared_ptr<inet::cpoll> gwPoll;
    cratelimiter rateLimiter;

    std::mutex gwConnectionLock;
    std::unordered_map<std::string, std::shared_ptr<cgateway>> gwConnections;
    std::shared_ptr<cwebhook> gwHook;

    core::casyncqueue retryQueue;

    struct gwcred_t {
        gwcred_t(const std::string& login, const std::string& pass, const std::vector<std::string>& hosts, const std::string& port) :
            login {login},
            pass {pass},
            hosts {hosts},
            port {port} {}
        const std::string login;
        const std::string pass;
        const std::vector<std::string> hosts;
        const std::string port;
    };

    struct addrs_t {
        addrs_t(std::string&& source, std::string&& destination) :
            source {std::move(source)},
            destination {std::move(destination)} {}
        const std::string source;
        const std::string destination;
    };

    class cackmsg : public std::enable_shared_from_this<cackmsg>, public inet::cpoll::cgc {
       public:
        cackmsg(const std::shared_ptr<csmppservice>& svc, const std::shared_ptr<gwcred_t>& gwcred, const std::shared_ptr<addrs_t>& addr, std::string&& msg, uint32_t id, uint32_t status, uint32_t seq) :
            svcWeak {svc},
            msg {std::move(msg)},
            id {id},
            status {status},
            sequence {seq},
            gwcred {gwcred},
            addr {addr},
            lastSeen {std::time(nullptr)} {}

        bool IsLeaveUs(std::time_t now) override;

       private:
        std::weak_ptr<csmppservice> svcWeak;

       public:
        const std::string msg;
        const uint32_t id {0}, status {0}, sequence {0};
        const std::shared_ptr<gwcred_t> gwcred;
        const std::shared_ptr<addrs_t> addr;
        std::atomic_bool finished {false};

       private:
        std::time_t lastSeen {0};
        size_t retryNum {0};
    };

    core::cspinlock retrySync;
    std::unordered_map<void*, std::shared_ptr<cackmsg>> retryMsgs;

    core::cspinlock ackSync;
    std::unordered_map<uint32_t, std::shared_ptr<cackmsg>> pendingMsgQueue;
    std::unordered_map<std::string, std::shared_ptr<cackmsg>> ackMsgQueue;

    const std::size_t numOfRetries {3};
    const std::time_t retryInterval {20};
};