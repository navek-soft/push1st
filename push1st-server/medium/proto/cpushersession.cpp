#include "cpushersession.h"
#include "../../core/csyslog.h"
#include "../cmessage.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../channels/cchannel.h"
#include <chrono>

void cpushersession::OnPusherSubscribe(const message_t& message) {
	auto&& data{ msg::ref(message) };
	if (data["data"].is_object() and data["data"]["channel"].is_string() and !data["data"]["channel"].empty()) {
		if (auto chName{ data["data"]["channel"].get<std::string>() }; !chName.empty() and chName.length() <= MaxChannelNameLength) {
			if (auto chType{ ChannelType(chName) };  chType == channel_t::type::priv and data["data"]["auth"].is_string()) {
				if (App->IsAllowChannel(chType, Id(), chName, data["data"]["auth"].get<std::string>())) {
					if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {
						SubscribedTo.emplace(chName, chSelf);
						chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
						WsWriteMessage(opcode_t::text, json::serialize({
							{"event","pusher_internal:subscription_succeeded"}, {"channel", chName}
						}));
						return;
					}
				}
			}
			else if (chType == channel_t::type::pres and data["data"]["auth"].is_string()) {
				SessionPresenceData = data["data"]["channel_data"].is_string() ? data["data"]["channel_data"].get<std::string>() : std::string{};
				if (App->IsAllowChannel(chType, Id(), chName, data["data"]["auth"].get<std::string>(), SessionPresenceData)) {
					if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {

						if (json::value_t ui; json::unserialize(SessionPresenceData, ui) and ui["user_id"].is_string()) { SessionUserId = ui["user_id"].get<std::string>(); }

						SubscribedTo.emplace(chName, chSelf);
						chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));

						usersids_t ids; userslist_t users;
						chSelf->GetUsers(ids, users);
						WsWriteMessage(opcode_t::text, json::serialize({
							{"event","pusher_internal:subscription_succeeded"}, {"channel", chName},
							{"data",json::serialize({ {"presence", json::object_t{{"ids",ids},{"hash",users},{"count",users.size()}}}})}
						}));
						return;
					}
				}
			}
			else if (chType == channel_t::type::pub and App->IsAllowChannel(chType, Id(), chName, {})) {
				if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {
					SubscribedTo.emplace(chName, chSelf);
					chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));

					usersids_t ids; userslist_t users;
					chSelf->GetUsers(ids, users);
					WsWriteMessage(opcode_t::text, json::serialize({
						{"event","pusher_internal:subscription_succeeded"}, {"channel", chName}
					}));
					return;
				}
			}
		}
	}
	WsError(close_t::ProtoError, -EPROTO);
}

void cpushersession::OnPusherUnSubscribe(const message_t& message) {
	auto&& data{ msg::ref(message) };
	if (data["data"].is_object() and data["data"]["channel"].is_string()) {
		if (auto&& chIt{ SubscribedTo.find(data["data"]["channel"].get<std::string>()) }; chIt != SubscribedTo.end()) {
			if (auto&& ch{ chIt->second.lock() }; ch) {
				ch->UnSubscribe(subsId);
			}
		}
	}
}

void cpushersession::OnPusherPing(const message_t& message) {
	syslog.print(7, "[ PUSHER:%s ] Ping ( ping )\n", Id().c_str());
	WsWriteMessage(opcode_t::text, json::serialize({ {"event","pusher:pong"} }));
}

void cpushersession::OnPusherPush(const message_t& message) {
	auto&& data{ msg::ref(message) };
	if (auto&& chName{ data["channel"].get<std::string>() }; !chName.empty()) {
		if (auto&& chIt{ SubscribedTo.find(data["data"]["channel"].get<std::string>()) }; chIt != SubscribedTo.end()) {
			if (auto&& ch{ chIt->second.lock() }; ch) {
				/*ch->Push(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
				channels.Pull(sesApp, chName, shared_from_this(),
					event(evName, chName, json::data(payload["data"]), {}, { sesId }));
				*/
			}
		}
		syslog.trace("[ PUSHER:%s ] Channel `%s` PULL\n", Id().c_str(), chName.c_str());
		return;
	}
	WsError(close_t::ProtoError, -EPROTO);
}

void cpushersession::OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& data, size_t length) {
	if (auto&& message{ UnPack(data, length) }; message) {
		auto&& msg{ msg::ref(message) };
		if (auto&& evName{ msg["event"].get<std::string>() }; !evName.empty()) {
			if (evName == "pusher:subscribe") {
				OnPusherSubscribe(message);
				return;
			}
			else if (evName == "pusher:ping") {
				OnPusherPing(message);
				return;
			}
			else if (evName == "pusher:unsubscribe") {
				OnPusherUnSubscribe(message);
				return;
			}
			else if (msg["channel"].is_string()) {
				OnPusherPush(message);
				return;
			}
		}
	}
	WsError(close_t::ProtoError, -EPROTO);
}

#if SENDQ 
void cpushersession::OnSocketSend() {
	message_t message;
	std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
	while (!OutgoingQueue.empty()) {
		message = OutgoingQueue.front();
		OutgoingQueue.pop();
		lock.unlock();
		WsWriteMessage(opcode_t::text, Pack(message));
		lock.lock();
	}
	SocketUpdateEvents(EPOLLIN | EPOLLRDHUP | EPOLLERR);
}
#endif

void cpushersession::OnSocketError(ssize_t err) {
	syslog.error("[ PUSHER:%s ] Error ( %s )\n", Id().c_str(), std::strerror((int)-err));
	for (auto&& it : SubscribedTo) {
		if (auto&& ch{ it.second.lock() }; ch) {
			ch->UnSubscribe(subsId);
		}
	}
	SocketClose();
}

void cpushersession::OnWsClose() {
	OnSocketError(-ENOTCONN);
}

bool cpushersession::OnWsConnect(const http::uri_t& path, const http::headers_t& headers) {
	syslog.trace("[ PUSHER:%ld:%s ] Connect\n", Fd(), Id().c_str());

	SetSendTimeout(500);

	return WsWriteMessage(opcode_t::text, json::serialize({
		{"event","pusher:connection_established"} ,
		{"data", json::serialize({ {"socket_id",Id()}, {"activity_timeout", std::to_string(KeepAlive)},})}
	}), false) == 0;
}

cpushersession::cpushersession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive) :
	inet::csocket{ std::move(fd) }, csubscriber{ GetAddress(), GetPort() },
	MaxMessageLength{ maxMessageLength }, KeepAlive{ keepAlive }, Channels{ channels }, App{ app }, EnablePushOnChannels{ pushOnChannels }
{
	//syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cpushersession::~cpushersession() {
	syslog.trace("[ PUSHER:%s ] Destroy\n", Id().c_str());
}

inline message_t cpushersession::UnPack(const std::shared_ptr<uint8_t[]>& data, size_t length) {
	try {
		return msg::unserialize({ data,length }, subsId);
	}
	catch (std::exception& ex) {
		syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), ex.what());
	}
	return {};
}