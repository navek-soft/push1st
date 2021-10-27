#include "cwssession.h"
#include "../../core/csyslog.h"
#include "../../core/ci/cjson.h"
#include "../cmessage.h"
#include "../ccredentials.h"
#include "../cchannels.h"
#include "../channels/cchannel.h"

message_t cwssession::UnPack(data_t&& data) {
	try {
		if (auto msg{ msg::unserialize(data,subsId) }; msg) {
			auto&& root{ *msg };
			if(root.is_object() and root.contains("channel") and root.contains("event") and root.contains("data") and !root["channel"].empty() and root["channel"].size() <= MaxChannelNameLength) {
				return msg;
			}
		}
		syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), "Invalid message format");
	}
	catch (std::exception& ex) {
		syslog.error("[ RAW:%s ] Message ( %s )\n", Id().c_str(), ex.what());
	}
	return {};
}

void cwssession::OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& data, size_t length) {
	try {
		ActivityCheckTime = std::time(nullptr) + KeepAlive;

		if (auto&& message{ UnPack({std::move(data),length}) }; message) {
			if (auto&& chIt{ SubscribedTo.find((*message)["channel"].get<std::string>()) }; chIt != SubscribedTo.end()) {
				if (std::shared_ptr<cchannel> ch{ chIt->second.lock() }; ch) {
					if (EnablePushOnChannels & ch->Type()) {
						ch->Push(std::move(message));
						App->Trigger(ch->Type(), hook_t::type::push, (*message)["channel"].get<std::string>(), Id(), std::move(*message));
						syslog.print(7, "[ RAW:%s ] Push ( %s/%s )\n", Id().c_str(), (*message)["channel"].get<std::string>().c_str(), (*message)["event"].get<std::string>().c_str());
					}
					else {
						syslog.print(7, "[ RAW:%s ] Push ( %s ) disable by config\n", Id().c_str(), (*message)["channel"].get<std::string>().c_str(), (*message)["event"].get<std::string>().c_str());
					}
					return;
				}
				else {
					syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror(EBADSLT));
					OnSocketError(-EBADSLT);
				}
			}
			else {
				syslog.print(4, "[ RAW:%s ] Push ( %s ) no channel subscription\n", Id().c_str(), (*message)["channel"].get<std::string>().c_str(), (*message)["event"].get<std::string>().c_str());
				OnSocketError(-EACCES);
			}
		}
		else {
			OnSocketError(-EBADMSG);
		}
	}
	catch (std::exception& ex) {
		syslog.error("%s ( %s )\n", __PRETTY_FUNCTION__, ex.what());
		OnSocketError(-EBADMSG);
	}
}

#if SENDQ 
void cwssession::OnSocketSend() {
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

void cwssession::OnSocketError(ssize_t err) {
	syslog.trace("[ RAW:%ld:%s ] Error ( %s ) (%ld)\n", Fd(), Id().c_str(), std::strerror((int)-err), SubscribedTo.size());
	for (auto&& it : SubscribedTo) {
		if (auto&& ch{ it.second.lock() }; ch) {
			ch->UnSubscribe(subsId);
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

bool cwssession::OnWsConnect(const http::uri_t& path, const http::headers_t& headers) {
	
	//syslog.trace("[ RAW:%ld:%s ] Connect\n", Fd(), Id().c_str());

	SetSendTimeout(500);
	SetKeepAlive(true, 2, 1, 1);

	size_t nchannels{ 0 };
	for (auto&& it{ path.uriPathList.begin() + 4 }; it != path.uriPathList.end(); ++it) {
		std::string chName{ *it };
		if (auto chType = ChannelType(*it); chType != channel_t::type::none and App->IsAllowChannelToken(chType, Id(), chName,
			http::GetValue(headers, "authorization", path.arg("token")), {}, std::string{ http::GetValue(headers, "origin") })) 
		{
			if (auto&& chSelf{ Channels->Register(chType, App, chName) }; chSelf) {
				SubscribedTo.emplace(chName, chSelf);
				chSelf->Subscribe(std::dynamic_pointer_cast<csubscriber>(shared_from_this()));
				++nchannels;
			}
		}
	}
	if (!nchannels) {
		syslog.trace("[ RAW:%ld:%s ] No channels subscriptions ( or invalid authorization )\n", Fd(), Id().c_str());
		WsClose(close_t::PolicyViolation);
		return false;
	}

	Poll()->EnqueueGc(shared_from_this());

	return true;
}

cwssession::cwssession(const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, size_t maxMessageLength, const channel_t& pushOnChannels, std::time_t keepAlive, const std::string& sessPrefix) :
	inet::csocket{ std::move(fd) }, csubscriber{ GetAddress(), GetPort(), sessPrefix },
	MaxMessageLength{ maxMessageLength }, KeepAlive{ keepAlive }, Channels{ channels }, App{ app }, EnablePushOnChannels{ pushOnChannels }
{
	ActivityCheckTime = std::time(nullptr) + KeepAlive;
	//syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
	//syslog.trace("[ RAW:%ld:%s ] New\n", Fd(), Id().c_str());
}

cwssession::~cwssession() {
	//syslog.trace("[ RAW:%ld:%s ] Destroy\n", Fd(), Id().c_str());
}