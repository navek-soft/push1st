#include "cwsrawserver.h"
#include "cwsrawconnection.h"
#include "../ccredentials.h"

ssize_t cwsrawserver::WsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers) {
	if (path.at(1) == Path) {
		if (auto&& App{ Credentials->GetAppByKey(std::string{path.at(2)}) }; App) {
			return cwsserver::WsUpgrade(fd, path, args, headers);
		}
	}
	return -EACCES;
}

inet::socket_t cwsrawserver::OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) {
	if (path.at(1) == Path) {
		if (auto&& App{ Credentials->GetAppByKey(std::string{path.at(2)}) }; App) {
			if (auto&& conn{ std::make_shared<cwsrawconnection>(Channels, App, fd, MaxMessageLength) }; conn and conn->OnWsConnect(path, args, headers)) {
				return std::dynamic_pointer_cast<inet::csocket>(conn);
			}
			else if(conn) {
				HttpWriteResponse(fd, "403");
				conn->SocketClose();
			}
			return {};
		}
		else {
			HttpWriteResponse(fd, "404");
		}
	}
	else {
		HttpWriteResponse(fd, "400");
	}
	fd.SocketClose();
	return {};
}

cwsrawserver::cwsrawserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, const config::server_t& config) :
	inet::cwsserver{ "ws:srv", std::string{ config.Listen.hostport() }, config.Ssl.Context(), 8192 },
	Channels{ channels }, Credentials{ credentials }, Path{ config.Path },
	MaxMessageLength{ config.MaxPayloadSize }
{
	syslog.ob.print("websocket.raw", "...enable %s proto, listen on %s, path %s", config.Ssl.Enable ? "https" : "http", std::string{config.Listen.hostport()}.c_str(), config.Path.c_str());
}

cwsrawserver::~cwsrawserver() {

}
