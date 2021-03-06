#include "../../../lib/http/cwsconn.h"
#include "../csubscriber.h"
#include "../../../lib/http/chttp.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../../core/ci/cspinlock.h"
#include "../cmessage.h"
#include <queue>

#ifndef SENDQ
#define SENDQ 0
#endif // !SENDQ


class cpushersession : public inet::cwsconnection, public inet::csocket, public csubscriber, public inet::cpoll::cgc, public std::enable_shared_from_this<cpushersession>
{
public:
	cpushersession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive);
	virtual ~cpushersession();

	virtual inline bool IsConnected(std::time_t now) { return !IsLeaveUs(now); }

	virtual inline bool IsLeaveUs(std::time_t now) override {
		ssize_t res{ 0 };
		if ((ActivityCheckTime + KeepAlive) >= now and (res = inet::GetErrorNo(Fd())) == 0) {
			return false;
		}
		WsError(websocket_t::close_t::GoingAway, res);
		return true;
	}

	virtual inline void Disconnect() override { OnSocketError(-ECONNRESET); }

	virtual bool OnWsConnect(const http::uri_t& path, const http::headers_t& headers);
	virtual inline void OnWsError(ssize_t err) override { 
		//printf("\t### %ld | %s ... CLOSE\n", std::time(nullptr), WsSessionId().c_str());
		OnSocketError(err); 
	}
	virtual inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) override { return SocketRecv(data, size, readed, flags); }
	virtual inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) override { return SocketSend(data, size, writen, flags); }
	virtual void OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& message, size_t length) override;

	virtual inline std::string WsSessionId() { return Id(); }

	virtual inline void OnSocketRecv() override { WsReadMessage(MaxMessageLength); }
#if SENDQ 
	virtual void OnSocketSend() override;
#else
	virtual inline void OnSocketSend() override { ; }
#endif
	virtual void OnSocketError(ssize_t err) override;

	virtual void OnWsClose() override;
	virtual inline void OnWsPing() override { ActivityCheckTime = std::time(nullptr) + KeepAlive; }
	virtual void GetUserInfo(std::string& userId, std::string& userData) override { userId = SessionUserId; userData = SessionPresenceData; }
	virtual inline fd_t GetFd() { return Fd(); }
	virtual inline ssize_t Push(const message_t& msg) override {
#if SENDQ
		std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
		OutgoingQueue.emplace(msg);
		SocketUpdateEvents(EPOLLOUT | EPOLLET);
#else
		auto res = WsSendMessage(opcode_t::text, Pack(msg));
		if (res == 0) {
			ActivityCheckTime = std::time(nullptr) + KeepAlive;
		}
		else {
			OnSocketError(res);
		}
		return res;
#endif
	}
private:
	inline void OnPusherSubscribe(const message_t& message);
	inline void OnPusherUnSubscribe(const message_t& message);
	inline void OnPusherPing(const message_t& message);
	inline void OnPusherPush(const message_t& message);
	inline std::string Pack(const message_t& msg);
	inline message_t UnPack(const std::shared_ptr<uint8_t[]>& data, size_t length);
	size_t MaxMessageLength{ 65536 };
	std::time_t KeepAlive{ 20 }, ActivityCheckTime{ 0 };
	std::shared_ptr<cchannels> Channels;
	std::unordered_map<std::string, std::weak_ptr<cchannel>> SubscribedTo;
	app_t App;
	channel_t EnablePushOnChannels{ channel_t::type::pres };
	std::string SessionUserId, SessionPresenceData;
#if SENDQ 
	spinlock_t OutgoingLock;
	std::queue<message_t> OutgoingQueue;
#endif
};

inline std::string cpushersession::Pack(const message_t& message) {
	return json::serialize({
		{"event", (*message)["event"]}, {"channel", (*message)["channel"]},
		{"#msg-time", std::chrono::system_clock::now().time_since_epoch().count() - (*message)["#msg-arrival"].get<size_t>()},
		{"data", (*message)["data"]}, {"#msg-id", (*message)["#msg-id"]}, {"#msg-from", (*message)["#msg-from"]}
	});
}