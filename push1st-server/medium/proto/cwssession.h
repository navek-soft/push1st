#include "../http/cwsconn.h"
#include "../csubscriber.h"
#include "../http/chttp.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../../core/ci/cspinlock.h"
#include <queue>
#include "../cmessage.h"

#ifndef SENDQ
#define SENDQ 0
#endif // !SENDQ


class cwssession : public inet::cwsconnection, public inet::csocket, public csubscriber, public std::enable_shared_from_this<cwssession>
{
public:
	cwssession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive);
	virtual ~cwssession();

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
	virtual inline void OnWsPing() override { ; }

	virtual void GetUserInfo(std::string& userId, std::string& userData) override { ; }
	virtual inline fd_t GetFd() { return Fd(); }
	virtual inline void Push(const message_t& msg) override {
#if SENDQ
		std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
		OutgoingQueue.emplace(msg);
		SocketUpdateEvents(EPOLLOUT | EPOLLET);
#else
		WsWriteMessage(opcode_t::text, Pack(msg));
#endif
	}
private:
	inline message_t UnPack(data_t&& data);
	inline std::string Pack(const message_t& msg);
	size_t MaxMessageLength{ 65536 };
	std::time_t KeepAlive{ 3600 }; 
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
	auto&& msg{ msg::ref(message) };
	return json::serialize({
		{"event", msg["event"]}, {"channel", msg["channel"]},
		{"#msg-time", std::chrono::system_clock::now().time_since_epoch().count() - msg["#msg-arrival"].get<size_t>()},
		{"data", msg["data"]}, {"#msg-id", msg["#msg-id"]}, {"#msg-from", msg["#msg-from"]}
	});
}