#include "../core/csyslog.h"
#include "capiserver.h"
#include "cmessage.h"
#include "ccredentials.h"
#include "cchannels.h"
#include "channels/cchannel.h"
#include <cstring>

static inline message_t dupChannelMessage(const json::value_t& msg, const std::string& channel) {
	message_t&& out{ std::make_shared<json::value_t>(json::object_t{}) };
	(*out)["channel"] = channel;
	(*out)["event"] = msg["name"];
	(*out)["data"] = msg["data"];
	(*out)["#msg-id"] = msg["#msg-id"];
	(*out)["#msg-from"] = msg["#msg-from"];
	(*out)["#msg-arrival"] = msg["#msg-arrival"];
	(*out)["#msg-expired"] = msg["#msg-expired"];
	(*out)["#msg-delivery"] = msg["#msg-delivery"];
	return out;
}

void capiserver::ApiOnEvents(const std::vector<std::string_view>& vals, const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, std::string&& content) {
	if (method == "POST") {
		auto&& tmStart{ std::chrono::system_clock::now().time_since_epoch().count() };
		if (auto&& app{ Credentials->GetAppById(std::string{vals[0]}) }; app) {
			if (message_t message; !content.empty() and (message = msg::unserialize(content, "api"))) {
				auto&& msg{ msg::ref(message) };
				if (msg.contains("channels") and msg["channels"].is_array() and msg.contains("name")) {
					json::object_t reply;
					reply["msg-id"] = msg["#msg-id"];
					reply["channels"] = json::object_t{};
					std::string chName;
					for (auto&& chIt : msg["channels"]) {
						chName = chIt.get<std::string>();
						if (auto&& ch{ Channels->Get(app,chName) }; ch) {
							reply["channels"][chName] = ch->Push(dupChannelMessage(msg, chName));
						}
						else {
							reply["channels"][chName] = (size_t)0;
						}
					}
					reply["time"] = std::chrono::system_clock::now().time_since_epoch().count() - tmStart;
					ApiResponse(fd, "200", json::serialize(reply), strncasecmp(http::GetValue(headers, "Connection", "Close").data(), "Keep-Alive", 10) == 0);
				}
				else {
					ApiResponse(fd, "400");
				}
			}
			else {
				ApiResponse(fd, "400");
			}
		}
		else {
			ApiResponse(fd, "403");
		}
	}
	else {
		ApiResponse(fd, "400");
	}
}

void capiserver::ApiResponse(const inet::csocket& fd, const std::string_view& code, const std::string& response, bool close) {
	if (close or KeepAliveTimeout.empty()) {
		HttpWriteResponse(fd, code, response, { {"Connection","Close"}, {"Content-Type", "application/json; charset=utf-8"} });
		fd.SocketClose();
	}
	else {
		HttpWriteResponse(fd, code, response, { {"Connection","Keep-Alive"}, {"Content-Type", "application/json; charset=utf-8"}, {"Keep-Alive", "timeout=" + KeepAliveTimeout} });
	}
}

void capiserver::OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, std::string&& request, std::string&& content) {
	if(!ApiRoutes.call(fd,method,path,headers,std::move(content))) {
		HttpWriteResponse(fd, "404");
		fd.SocketClose();
	}
}

void capiserver::Listen(const std::shared_ptr<inet::cpoll>& poll) 
{
	if (ApiTcp) { ApiTcp->Listen(poll); }
	if (ApiUnix) { ApiUnix->Listen(poll); }
}


capiserver::capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::interface_t config) :
	Channels{ channels }, Credentials{ credentials }, KeepAliveTimeout{ std::to_string(config.KeepAlive) }
{
	if (!config.Tcp.empty()) {
		ApiTcp = std::make_shared<ctcpapiserver>(*this, config);
	}
	if (!config.Unix.empty()) {
		ApiUnix = std::make_shared<cunixapiserver>(*this, config);
	}

	ApiRoutes.add("/" + config.Path + "/?/events/", std::bind(&capiserver::ApiOnEvents, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
}

capiserver::~capiserver() {

}

capiserver::ctcpapiserver::ctcpapiserver(capiserver& api, config::interface_t config) :
	inet::chttpserver{ "tapi:srv", std::string{ config.Tcp.hostport() }, config.Ssl.Context(), MaxHttpHeaderSize }, Api{ api }
{
	syslog.ob.print("Api", "Web ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Tcp.hostport() }.c_str(), config.Path.c_str());
}

capiserver::ctcpapiserver::~ctcpapiserver() {
	
}

ssize_t capiserver::cunixapiserver::OnUnixAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		return self->PollAdd(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR, std::bind(&cunixapiserver::OnHttpData, this, std::placeholders::_1, std::placeholders::_2, sa, ssl, poll));
	}
	return res;
}

void capiserver::cunixapiserver::OnHttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		std::string_view method; http::uri_t path; http::headers_t headers;  std::string request, content;
		inet::csocket so(fd, sa, ssl, poll);
		if (res = HttpReadRequest(so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
			OnHttpRequest(so, method, path, headers, std::move(request), std::move(content));
		}
		else {
			so.SocketClose();
		}
		return;
	}
	inet::Close(fd);
}

capiserver::cunixapiserver::cunixapiserver(capiserver& api, config::interface_t config, size_t httpMaxHeaderSize) :
	inet::cunixserver{ "uapi:srv", false, 30 }, Api{ api }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	std::filesystem::path saFile{ config.Unix.path() };

	std::filesystem::create_directories(saFile.parent_path());

	if (auto res = UnixListen(config.Unix.url(), true, 16); res == 0) {
		syslog.ob.print("Api", "Unix ... enable, listen on %s, route /%s/", saFile.c_str(), config.Path.c_str());
	}
	else {
		syslog.error("Unix API Initialize server error ( %s )\n", std::strerror(-(int)res));
		throw std::runtime_error("Unix API server exception");
	}
}

capiserver::cunixapiserver::~cunixapiserver() {
	UnixClose();
}