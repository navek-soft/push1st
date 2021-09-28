#include "cwssession.h"
#include "../../core/csyslog.h"
#include "../../core/ci/cjson.h"
#include "../cmessage.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../channels/cchannel.h"


std::unique_ptr<cmessage> cwssession::UnPack(data_t&& msg) {
	std::unique_ptr<cmessage> unpack;
	std::string err;
	json::value_t root;
	try {
		if (json::unserialize(to_string(msg), root, err)) {
			if(root.isObject() and root.isMember("channel")) {
				delivery_t delivery{ delivery_t::broadcast };
				std::time_t ttl{ 0 };

				if (root.isMember("options") and root.isObject()) {
					if (root["options"].isMember("delivery")) {
						if (auto&& value{ root["options"]["delivery"] }; value == "multicast") {
							delivery = delivery_t::multicast;
						}
						else if (value == "unicast") {
							delivery = delivery_t::unicast;
						}
					}
					if (root["options"].isMember("ttl")) {
						ttl = (std::time_t)root["options"]["ttl"].asInt();
					}
				}
				unpack = std::move(std::make_unique<cmessage>(msg, 
					root.isMember("event") ? root["event"].asString() : std::string{}, root["channel"].asString(), Id(), delivery, ttl));
			}
			else {
				syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), "Invalid message format");
			}
		}
		else {
			syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), err.c_str());
		}
	}
	catch (std::exception& ex) {
		syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), ex.what());
	}
	return unpack;
}

void cwssession::OnWsMessage(websocket_t::opcode_t opcode, std::shared_ptr<uint8_t[]>&& data, size_t length) {
	if (auto&& message{ UnPack({std::move(data),length}) }; message and !message->Channel.empty()) {
		if (auto&& chIt{ SubscribedTo.find(message->Channel) }; chIt != SubscribedTo.end()) {
			if (std::shared_ptr<cchannel> ch{ chIt->second.lock() }; ch) {
				if (EnablePushOnChannels & ch->Type()) {
					ch->Push(std::move(message));
					App->Trigger(hook_t::type::push, message->Channel, Id(), message->Data);
					syslog.print(4, "[ RAW:%s ] Push ( %s/%s )\n", Id().c_str(), message->Channel.c_str(), message->Event.c_str());
				}
				else {
					syslog.print(4, "[ RAW:%s ] Push ( %s ) disable by config\n", Id().c_str(), message->Channel.c_str(), message->Event.c_str());
				}
			}
			else {
				syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror(EBADSLT));
			}
			return;
		}
		else {
			syslog.print(4, "[ RAW:%s ] Push ( %s ) no channel subscription\n", Id().c_str(), message->Channel.c_str(), message->Event.c_str());
		}
	}
}
#if SENDQ 
void cwssession::OnSocketSend() {
	std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
	while (!OutgoingQueue.empty()) {
		WsWriteMessage(opcode_t::text, { (char*)OutgoingQueue.front().first.get(),OutgoingQueue.front().second });
		OutgoingQueue.pop();
	}
	SocketUpdateEvents(EPOLLIN | EPOLLRDHUP | EPOLLERR);
}
#endif

void cwssession::OnSocketError(ssize_t err) {
	syslog.error("[ RAW:%s ] Error ( %s )\n", Id().c_str(), std::strerror((int)-err));
	for (auto&& it : SubscribedTo) {
		if (auto&& ch{ it.second.lock() }; ch) {
			ch->UnSubscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
		}
	}
	SocketClose();
}

void cwssession::OnWsClose() {
	OnSocketError(-ENOTCONN);
}

static inline std::pair<std::string_view, std::string_view> ExplodePathName(std::string_view Path) {
	std::pair<std::string_view, std::string_view> value;
	if (auto npos{ Path.find_first_of('=') }; npos != std::string_view::npos) {
		value.first = Path.substr(0, npos);
		value.second = Path.substr(npos+1);
	}
	else {
		value.first = Path;
	}
	return value.first.size() <= MaxChannelNameLength ? value : std::pair<std::string_view, std::string_view>{ {}, {}};
}

bool cwssession::OnWsConnect(const http::path_t& path, const http::params_t& args, const http::headers_t& headers) {
	
	syslog.trace("[ RAW:%ld:%s ] Connect\n", Fd(), Id().c_str());

	size_t nchannels{ 0 };
	for (size_t n{ 3 }; n < path.size(); ++n) {
		if (auto&& [chName, chToken] = ExplodePathName(path.at(n)); !chName.empty()) {
			if (auto chType = ChannelType(path.at(n)); chType != channel_t::type::none and App->IsAllowChannel(chType, Id(), chName, chToken)) {
				if (auto&& chSelf{ Channels->Register(chType, App, std::string{chName}) }; chSelf) {
					SubscribedTo.emplace(chName,chSelf);
					chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
					++nchannels;
				}
			}
		}
	}
	return nchannels;
}

cwssession::cwssession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive) :
	inet::csocket{ std::move(fd) }, csubscriber{ GetAddress(), GetPort() }, 
	MaxMessageLength{ maxMessageLength }, KeepAlive{ keepAlive }, Channels{ channels }, App{ app }, EnablePushOnChannels{ pushOnChannels }
{
	//syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cwssession::~cwssession() {
	syslog.trace("[ RAW:%s ] Destroy\n", Id().c_str());
}