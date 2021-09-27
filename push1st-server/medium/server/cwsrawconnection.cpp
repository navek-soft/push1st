#include "cwsrawconnection.h"
#include "../../core/ci/cjson.h"
#include "../cmessage.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../channels/cchannel.h"


std::unique_ptr<cmessage> cwsrawconnection::UnPack(data_t&& msg) {
	std::unique_ptr<cmessage> unpack;
	Json::CharReaderBuilder builder;
	JSONCPP_STRING err;
	Json::Value root;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	try {
		if (reader->parse((char*)msg.first.get(), (char*)(msg.first.get() + msg.second), &root, &err)) {
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
				syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, "Invalid message format");
			}
		}
		else {
			syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, err.c_str());
		}
	}
	catch (std::exception& ex) {
		syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, ex.what());
	}
	return unpack;
}

void cwsrawconnection::OnWsMessage(websocket_t::opcode_t opcode, std::shared_ptr<uint8_t[]>&& data, size_t length) {
	if (auto&& message{ UnPack({std::move(data),length}) }; message and !message->Channel.empty()) {
		if (auto&& chIt{ SubscribedTo.find(message->Channel) }; chIt != SubscribedTo.end()) {
			if (std::shared_ptr<cchannel> ch{ chIt->second.lock() }; ch) {
				ch->Push(std::move(message));
				App->Trigger(hook_t::type::push, message->Channel, Id(), message->Data);
			}
			else {
				syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror(EBADSLT));
			}
			return;
		}
	}
	syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror(EBADMSG));
}

void cwsrawconnection::OnSocketSend() {
	{
		std::unique_lock<decltype(OutgoingLock)> lock(OutgoingLock);
		while (!OutgoingQueue.empty()) {
			WsWriteMessage(opcode_t::text, { (char*)OutgoingQueue.front().first.get(),OutgoingQueue.front().second });
			OutgoingQueue.pop();
		}
	}
	SocketUpdateEvents(EPOLLIN | EPOLLRDHUP);
}

void cwsrawconnection::OnSocketError(ssize_t err) {
	syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror((int)-err));
	for (auto&& it : SubscribedTo) {
		if (auto&& ch{ it.second.lock() }; ch) {
			ch->UnSubscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
		}
	}
	SocketClose();
}

void cwsrawconnection::OnWsClose() {
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
	return value;
}

bool cwsrawconnection::OnWsConnect(const http::path_t& path, const http::params_t& args, const http::headers_t& headers) {
	size_t nchannels{ 0 };
	for (size_t n{ 3 }; n < path.size(); ++n) {
		if (auto&& [chName, chToken] = ExplodePathName(path.at(n)); !chName.empty()) {
			if (auto chType = ChannelType(path.at(n)); chType != channel_t::type::none and App->IsAllowChannel(chType, chName, chToken)) {
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

cwsrawconnection::cwsrawconnection(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength) :
	inet::csocket{ std::move(fd) }, csubscriber{ GetAddress(), GetPort() }, MaxMessageLength{ maxMessageLength }, Channels{ channels }, App{ app }
{
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cwsrawconnection::~cwsrawconnection() {
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}