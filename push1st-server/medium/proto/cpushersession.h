#include "../http/cwsconn.h"
#include "../csubscriber.h"
#include "../http/chttp.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../../core/ci/cspinlock.h"
#include "../../core/ci/cjson.h"
#include "../cmessage.h"
#include <queue>

#ifndef SENDQ
#define SENDQ 1
#endif // !SENDQ


class cpushersession : public inet::cwsconnection, public inet::csocket, public csubscriber, public std::enable_shared_from_this<cpushersession>
{
public:
	cpushersession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive);
	virtual ~cpushersession();

	virtual bool OnWsConnect(const http::path_t& path, const http::params_t& args, const http::headers_t& headers);
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
	virtual void GetUserInfo(std::string& userId, std::string& userData) override { userId = SessionUserId; userData = SessionPresenceData; }
	virtual void Push(const std::unique_ptr<cmessage>& msg) override;
private:
	inline void OnPusherSubscribe(const json::value_t& data);
	inline void OnPusherUnSubscribe(const json::value_t& data);
	inline void OnPusherPing(const json::value_t& data);
	inline void OnPusherPush(const json::value_t& data);
	inline bool UnPack(json::value_t& message, const std::shared_ptr<uint8_t[]>& data, size_t length);
	size_t MaxMessageLength{ 65536 };
	std::time_t KeepAlive{ 20 };
	std::shared_ptr<cchannels> Channels;
	std::unordered_map<std::string, std::weak_ptr<cchannel>> SubscribedTo;
	app_t App;
	channel_t EnablePushOnChannels{ channel_t::type::pres };
	std::string SessionUserId, SessionPresenceData;
#if SENDQ 
	spinlock_t OutgoingLock;
	std::queue<std::string> OutgoingQueue;
#endif
};