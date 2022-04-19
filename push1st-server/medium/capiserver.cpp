#include "../core/csyslog.h"
#include "capiserver.h"
#include "cmessage.h"
#include "ccredentials.h"
#include "cchannels.h"
#include "ccluster.h"
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
	if(msg.contains("socket_id"))
		(*out)["socket_id"] = msg["socket_id"];
	if (msg.contains("to_socket_id"))
		(*out)["to_socket_id"] = msg["to_socket_id"];
	return out;
}

void capiserver::ApiOnEvents(const std::vector<std::string_view>& vals, const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
	if (method == "POST") {
		auto&& tmStart{ std::chrono::system_clock::now().time_since_epoch().count() };
		if (auto&& app{ Credentials->GetAppById(std::string{vals[0]}) }; app) {
			if (message_t message; !content.empty() and (message = msg::unserialize(content, "api"))) {
//				auto&& msg{ msg::ref(message) };
				if ((*message).contains("channels") and (*message)["channels"].is_array() and (*message).contains("name")) {
					json::object_t reply;
					reply["msg-id"] = (*message)["#msg-id"];
					reply["channels"] = json::object_t{};
					std::string chName;
					for (auto&& chIt : (*message)["channels"]) {
						chName = chIt.get<std::string>();
						if (auto&& ch{ Channels->Get(app,chName) }; ch) {
							reply["channels"][chName] = ch->Push(dupChannelMessage((*message), chName));
						}
						else {
							reply["channels"][chName] = (size_t)0;
						}
						Cluster->Push(ChannelType(chName),app,chName, dupChannelMessage((*message), chName));
					}
					reply["time"] = std::chrono::system_clock::now().time_since_epoch().count() - tmStart;
					ApiResponse(fd, "200", json::serialize(reply), http::IsConnectionKeepAlive(headers));
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

void capiserver::ApiOnChannels(const std::vector<std::string_view>& vals, const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
	if (method == "GET") {
		if (auto&& app{ Credentials->GetAppById(std::string{vals[0]}) }; app) {
			if (vals[1].empty()) {
				json::array_t chList;
				for (auto&& [chName, chObj] : Channels->List()) {
					/* All channels list, filter by app-id */
					if (std::strncmp(app->Id.data(), chName.data(), app->Id.length()) == 0) {
						chList.emplace_back(chObj->ApiStats());
					}
				}
				ApiResponse(fd, "200", json::serialize(chList), http::IsConnectionKeepAlive(headers));
			}
			else if (auto&& ch{ Channels->Get(app,std::string{vals[1]}) }; ch) {
				ApiResponse(fd, "200", json::serialize(ch->ApiOverview()), http::IsConnectionKeepAlive(headers));
			}
			else {
				ApiResponse(fd, "404");
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

void capiserver::ApiOnToken(const std::vector<std::string_view>& vals, const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
	if (method == "POST") {
		if (auto&& app{ Credentials->GetAppById(std::string{vals[0]}) }; app) {
			if (vals[1] == "session") {
				if (json::value_t request; cjson::unserialize(content, request) and request.is_object() and request.contains("session") and request.contains("channel")) {
					auto&& token{ app->SessionToken(request["session"].get<std::string>(), request["channel"].get<std::string>(), !request.contains("data") ? std::string{} : request["data"].get<std::string>()) };
					ApiResponse(fd, "200", token);
				}
				else {
					ApiResponse(fd, "400");
				}
			}
			else if (vals[1] == "access") {
				if (json::value_t request; cjson::unserialize(content, request) and request.is_object() and request.contains("origin") and request.contains("channel") and request.contains("ttl")) {
					auto&& token{ app->AccessToken(request["origin"].get<std::string>(), request["channel"].get<std::string>(), request["ttl"].is_number() ? request["ttl"].get<size_t>() : 0) };
					ApiResponse(fd, "200", token);
				}
				else {
					ApiResponse(fd, "400");
				}
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

void capiserver::ApiOnWebHook(const std::vector<std::string_view>& vals, const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
	ApiResponse(fd, "200", {}, http::IsConnectionKeepAlive(headers));
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
void capiserver::OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, const std::string& request, const std::string_view& content) {

	syslog.print(4, "[ API:%ld:%s ] %s\n", fd.Fd(), std::string{ method }.c_str(), std::string{ path.uriFull }.c_str());

	if(!ApiRoutes.call(fd,method,path,headers, content)) {
		HttpWriteResponse(fd, "404");
		fd.SocketClose();
	}
}

void capiserver::Listen(const std::shared_ptr<inet::cpoll>& poll) 
{
	if (ApiTcp) { ApiTcp->Listen(poll); }
	if (ApiUnix) { ApiUnix->Listen(poll); }
}


capiserver::capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, const std::shared_ptr<ccluster>& cluster, config::interface_t config) :
	Channels{ channels }, Credentials{ credentials }, Cluster{ cluster }, KeepAliveTimeout{ std::to_string(config.KeepAlive) }
{
	if (!config.Tcp.empty()) {
		ApiTcp = std::make_shared<ctcpapiserver>(*this, config);
	}
	if (!config.Unix.empty()) {
		ApiUnix = std::make_shared<cunixapiserver>(*this, config);
	}

	/* Push event endpoint */
	ApiRoutes.add("/" + config.Path + "/?/events/*", std::bind(&capiserver::ApiOnEvents, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	/* Generate private\presense session token */
	ApiRoutes.add("/" + config.Path + "/?/token/?/", std::bind(&capiserver::ApiOnToken, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	ApiRoutes.add("/" + config.Path + "/?/channels/?/", std::bind(&capiserver::ApiOnChannels, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

#ifdef DEBUG
	ApiRoutes.add("/webhook/*", std::bind(&capiserver::ApiOnWebHook, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
#endif // DEBUG

}

capiserver::~capiserver() {

}

void capiserver::ctcpapiserver::OnHttpError(const inet::csocket& fd, ssize_t err) {
	syslog.print(7,"[ API ] tcp server request error ( %s )\n", std::strerror(-(int)err));
	fd.SocketClose();
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

void capiserver::cunixapiserver::OnHttpError(const inet::csocket& fd, ssize_t err) {
	syslog.error("[ API ] unix server request error ( %s )\n", std::strerror(-(int)err));
	fd.SocketClose();
}

void capiserver::cunixapiserver::OnHttpData(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		std::string_view method, content; http::uri_t path; http::headers_t headers;  std::string request;
		inet::csocket so(fd, sa, ssl, poll);
		if (res = HttpReadRequest(so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
			OnHttpRequest(so, method, path, headers, std::move(request), content);
		}
		else {
			OnHttpError(so, res);
		}
		return;
	}
	inet::Close(fd);
}

capiserver::cunixapiserver::cunixapiserver(capiserver& api, config::interface_t config, size_t httpMaxHeaderSize) :
	inet::cunixserver{ "uapi:srv", false, 30 }, Api{ api }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	try {
		std::filesystem::path saFile{ config.Unix.path() };

		std::filesystem::create_directories(saFile.parent_path());

		if (auto res = UnixListen(config.Unix.path(), true, 16); res == 0) {
			syslog.ob.print("Api", "Unix ... enable, listen on %s, route /%s/", saFile.c_str(), config.Path.c_str());
		}
		else {
			syslog.error("Unix API Initialize server error ( %s )\n", std::strerror(-(int)res));
			throw std::runtime_error("Unix API server exception");
		}
	}
	catch (std::exception& ex) {
		syslog.error("Unix API Initialize server error ( %s )\n", ex.what());
		throw std::runtime_error("Unix API server exception");
	}
}

capiserver::cunixapiserver::~cunixapiserver() {
	UnixClose();
}