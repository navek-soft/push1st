#include "../../../lib/http/cwsconn.h"
#include "../../../lib/http/chttp.h"
#include "../csubscriber.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../../core/ci/cspinlock.h"
#include <queue>
#include "../cmessage.h"

#ifndef SENDQ
#define SENDQ 0
#endif // !SENDQ


class cwssession : public inet::cwsconnection, public inet::csocket, public csubscriber, public inet::cpoll::cgc, public std::enable_shared_from_this<cwssession>
{
public:
	cwssession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive, const std::string& sessPrefix = {});
	virtual ~cwssession();

	virtual inline bool IsConnected(std::time_t now) { return !IsLeaveUs(now); }

	virtual inline bool IsLeaveUs(std::time_t now) override { 
		if (ActivityCheckTime >= now and inet::GetErrorNo(Fd()) == 0) {
			return false;
		}
		OnSocketError(-ETIMEDOUT);
		return true;
	}

	virtual bool OnWsConnect(const http::uri_t& path, const http::headers_t& headers);
	virtual inline void OnWsError(ssize_t err) override { OnSocketError(err); }
	virtual inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) override { return SocketRecv(data, size, readed, flags); }
	virtual inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) override { return SocketSend(data, size, writen, flags); }
	virtual void OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& message, size_t length) override;
	virtual inline void OnSocketRecv() override { WsReadMessage(MaxMessageLength); }
#if SENDQ 
	virtual void OnSocketSend() override;
#else
	virtual inline void OnSocketSend() override { ; }
#endif
	virtual void OnSocketError(ssize_t err) override;

	virtual void OnWsClose() override;
	virtual inline void OnWsPing() override { ActivityCheckTime = std::time(nullptr) + KeepAlive + 5; }

	virtual void GetUserInfo(std::string& userId, std::string& userData) override { ; }
	virtual inline fd_t GetFd() { return Fd(); }
	virtual inline ssize_t Push(const message_t& msg) override {
#if SENDQ
		std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
		OutgoingQueue.emplace(msg);
		SocketUpdateEvents(EPOLLOUT | EPOLLET);
#else
		auto res = WsSendMessage(opcode_t::text, Pack(msg));
		if (res == 0) {
			ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
		}
		else {
			OnSocketError(res);
		}
		return res;
#endif
	}
private:
	inline message_t UnPack(data_t&& data);
	inline std::string Pack(const message_t& msg);
	size_t MaxMessageLength{ 65536 };
	std::time_t KeepAlive{ 3600 }, ActivityCheckTime{ 0 };
	std::shared_ptr<cchannels> Channels;
	std::unordered_map<std::string, std::weak_ptr<cchannel>> SubscribedTo;
	app_t App;
	channel_t EnablePushOnChannels{ channel_t::type::pub | channel_t::type::prot | channel_t::type::pres };
#if SENDQ 
	spinlock_t OutgoingLock;
	std::queue<message_t> OutgoingQueue;
#endif
};


inline std::string cwssession::Pack(const message_t& message) {
	return json::serialize({
		{"event", (*message)["event"]}, {"channel", (*message)["channel"]},
		{"#msg-time", std::chrono::system_clock::now().time_since_epoch().count() - (*message)["#msg-arrival"].get<size_t>()},
		{"data", (*message)["data"]}, {"#msg-id", (*message)["#msg-id"]}, {"#msg-from", (*message)["#msg-from"]}
	});
}