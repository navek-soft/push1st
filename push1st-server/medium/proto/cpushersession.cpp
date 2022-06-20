#include "cpushersession.h"
#include "../../core/csyslog.h"
#include "../cmessage.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../channels/cchannel.h"
#include <chrono>

void cpushersession::OnPusherSubscribe(const message_t& message) {
	if ((*message)["data"].is_object() and (*message)["data"]["channel"].is_string() and !(*message)["data"]["channel"].empty()) {
		if (auto chName{ (*message)["data"]["channel"].get<std::string>() }; !chName.empty() and chName.length() <= MaxChannelNameLength) {
			if (auto chType{ ChannelType(chName) };  chType == channel_t::type::priv and (*message)["data"]["auth"].is_string()) {
				if (App->IsAllowChannelSession(chType, Id(), chName, (*message)["data"]["auth"].get<std::string>())) {
					if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {
						SubscribedTo.emplace(chName, chSelf);
						chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
						if (WsWriteMessage(opcode_t::text, json::serialize({
							{"event","pusher_internal:subscription_succeeded"}, {"channel", chName}
							})) == 0) 
						{
							ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
						}
						return;
					}
				}
			}
			else if (chType == channel_t::type::pres and (*message)["data"]["auth"].is_string()) {
				SessionPresenceData = (*message)["data"]["channel_data"].is_string() ? (*message)["data"]["channel_data"].get<std::string>() : std::string{};
				if (App->IsAllowChannelSession(chType, Id(), chName, (*message)["data"]["auth"].get<std::string>(), SessionPresenceData)) {
					if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {

						if (json::value_t ui; json::unserialize(SessionPresenceData, ui) and ui["user_id"].is_string()) { SessionUserId = ui["user_id"].get<std::string>(); }

						SubscribedTo.emplace(chName, chSelf);
						chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));

						usersids_t ids; userslist_t users;
						chSelf->GetUsers(ids, users);
						if (WsWriteMessage(opcode_t::text, json::serialize({
							{"event","pusher_internal:subscription_succeeded"}, {"channel", chName},
							{"data",json::serialize({ {"presence", json::object_t{{"ids",ids},{"hash",users},{"count",users.size()}}}})}
							})) == 0) 
						{
							ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
						}
						return;
					}
				}
			}
			else if (chType == channel_t::type::pub and App->IsAllowChannelSession(chType, Id(), chName, {})) {
				if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {
					SubscribedTo.emplace(chName, chSelf);
					chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));

					usersids_t ids; userslist_t users;
					chSelf->GetUsers(ids, users);
					if (WsWriteMessage(opcode_t::text, json::serialize({
						{"event","pusher_internal:subscription_succeeded"}, {"channel", chName}
						})) == 0) 
					{
						ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
					}
					return;
				}
			}
		}
	}
	WsError(close_t::ProtoError, -EPROTO);
}

void cpushersession::OnPusherUnSubscribe(const message_t& message) {
	if ((*message)["data"].is_object() and (*message)["data"]["channel"].is_string()) {
		if (auto&& chIt{ SubscribedTo.find((*message)["data"]["channel"].get<std::string>()) }; chIt != SubscribedTo.end()) {
			if (auto&& ch{ chIt->second.lock() }; ch) {
				ch->UnSubscribe(subsId);
			}
		}
	}
}

void cpushersession::OnPusherPing(const message_t& message) {
	ssize_t res;
	if (res = WsWriteMessage(opcode_t::text, json::serialize({ {"event","pusher:pong"} })); res == 0) {
		ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
	}

	syslog.print(7, "[ PUSHER:%s ] Ping ( %s )\n", Id().c_str(),strerror(-(int)res));
}

void cpushersession::OnPusherPush(const message_t& message) {
	if (auto&& chName{ (*message)["channel"].get<std::string>() }; !chName.empty()) {
		if (auto&& chIt{ SubscribedTo.find((*message)["data"]["channel"].get<std::string>()) }; chIt != SubscribedTo.end()) {
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
		ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;

		if (auto&& evName{ (*message)["event"].get<std::string>() }; !evName.empty()) {
			if (evName == "pusher:subscribe") {
				OnPusherSubscribe(message);
				return;
			}
			else if (evName == "pusher:ping") {
				//printf("===\n%s\n===\n", std::string{ (char*)data.get(),length }.c_str());
				OnPusherPing(message);
				return;
			}
			else if (evName == "pusher:unsubscribe") {
				OnPusherUnSubscribe(message);
				return;
			}
			else if ((*message)["channel"].is_string()) {
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
	SetKeepAlive(true, 2, 1, 1);

	if (WsWriteMessage(opcode_t::text, json::serialize({
		{"event","pusher:connection_established"} ,
		{"data", json::serialize({ {"socket_id",Id()}, {"activity_timeout", std::to_string(KeepAlive)},})}
		}), false) == 0) {
		Poll()->EnqueueGc(shared_from_this());
		return true;
	}
	return false;
}

cpushersession::cpushersession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive) :
	inet::csocket{ std::move(fd) }, csubscriber{ GetAddress(), GetPort() },
	MaxMessageLength{ maxMessageLength }, KeepAlive{ keepAlive }, Channels{ channels }, App{ app }, EnablePushOnChannels{ pushOnChannels }
{
	ActivityCheckTime = std::time(nullptr) + KeepAlive + 5;
	//syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cpushersession::~cpushersession() {
	syslog.trace("[ PUSHER:%s ] Destroy\n", Id().c_str());
	for (auto&& it : SubscribedTo) {
		if (auto&& ch{ it.second.lock() }; ch) {
			ch->UnSubscribe(subsId);
		}
	}
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