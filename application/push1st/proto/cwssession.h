#include <queue>

#include "channels/cchannels.h"
#include "core/config/cconfig.h"
#include "core/credentials/ccredentials.h"
#include "core/subscriber/csubscriber.h"
#include "http/cwsconn.h"
#include "log/clog.h"

#ifndef SENDQ1
#define SENDQ1 0
#endif// !SENDQ

class cwssession : public inet::cwsconnection, public inet::csocket, public csubscriber, public inet::cpoll::cgc, public std::enable_shared_from_this<cwssession> {
    log_as(wssession);

   public:
    cwssession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive, const std::string& sessPrefix = {});
    ~cwssession() override;

    inline bool IsConnected(std::time_t now) override {
        return !IsLeaveUs(now);
    }

    inline bool IsLeaveUs(std::time_t now) override {
        if (ActivityCheckTime >= now and inet::GetErrorNo(Fd()) == 0) {
            return false;
        }
        return true;
    }
    inline void Disconnect() override {
        OnSocketError(-ECONNRESET);
    }
    bool OnWsConnect(const http::uri_t& path, const http::headers_t& headers) override;
    inline void OnWsError(ssize_t err) override {
        OnSocketError(err);
    }
    inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) override {
        return SocketRecv(data, size, readed, flags);
    }
    inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) override {
        return SocketSend(data, size, writen, flags);
    }
    void OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& data, size_t length) override;
    inline void OnSocketRecv() override {
        WsReadMessage(MaxMessageLength);
    }

    inline std::string WsSessionId() override {
        return Id();
    }

#if SENDQ1
    void OnSocketSend() override;
#else
    inline void OnSocketSend() override {
        ;
    }
#endif
    void OnSocketError(ssize_t err) override;

    void OnWsClose() override;
    inline void OnWsPing() override {
        ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
    }

    void GetUserInfo([[maybe_unused]] std::string& userId, [[maybe_unused]] std::string& userData) override {
        ;
    }
    inline fd_t GetFd() override {
        return Fd();
    }
    inline ssize_t Push(const message_t& msg) override {
#if SENDQ1
        std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
        if (OutgoingQueue.size() < 10) {
            OutgoingQueue.emplace(msg);
            SocketUpdateEvents(EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT);
            return 0;
        }
#else
        auto res = WsSendMessage(opcode_t::text, Pack(msg), false);
        if (res == 0) {
            ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
        } else {
            OnSocketError(res);
        }
        return res;
#endif
    }

   private:
    inline message_t UnPack(data_t&& data);
    inline std::string Pack(const message_t& message);
    size_t MaxMessageLength {65536};
    std::time_t KeepAlive {3600}, ActivityCheckTime {0};
    std::shared_ptr<cchannels> Channels;
    std::unordered_map<std::string, std::weak_ptr<cchannel>> SubscribedTo;
    app_t App;
    channel_t EnablePushOnChannels {channel_t::type::pub | channel_t::type::prot | channel_t::type::pres};
#if SENDQ1
    spinlock_t OutgoingLock;
    std::queue<message_t> OutgoingQueue;
#endif
};

inline std::string cwssession::Pack(const message_t& message) {// NOLINT (static)
    return json::Serialize(
        {{"event", (*message)["event"]},
         {"channel", (*message)["channel"]},
         {"#msg-time", std::chrono::system_clock::now().time_since_epoch().count() - (*message)["#msg-arrival"].get<size_t>()},
         {"data", (*message)["data"]},
         {"#msg-id", (*message)["#msg-id"]},
         {"#msg-from", (*message)["#msg-from"]}});
}