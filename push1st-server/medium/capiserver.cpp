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

capiserver::ctcpapiserver::ctcpapiserver(capiserver& api, config::interface_t config) :
	inet::chttpserver{ "ws:srv", std::string{ config.Tcp.hostport() }, config.Ssl.Context(), 8192 }, Api{ api }
{
	syslog.ob.print("Api", "Web ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Tcp.hostport() }.c_str(), config.Path.c_str());
}

capiserver::ctcpapiserver::~ctcpapiserver() {

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
}


capiserver::capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::interface_t config) :
	Channels{ channels }, Credentials{ credentials }, KeepAliveTimeout{ std::to_string(config.KeepAlive) }
{
	if (!config.Tcp.empty()) {
		ApiTcp = std::make_shared<ctcpapiserver>(*this, config);
	}
	if (ApiTcp) {
		ApiRoutes.add("/" + config.Path + "/?/events/", std::bind(&capiserver::ApiOnEvents, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	}
}

capiserver::~capiserver() {

}
