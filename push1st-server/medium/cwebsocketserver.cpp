#include "../core/csyslog.h"
#include "cwebsocketserver.h"
#include "proto/cwssession.h"
#include "proto/cpushersession.h"
#include "ccredentials.h"

ssize_t cwebsocketserver::WsUpgrade(const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) {
	if (auto&& proto{ ProtoRoutes.find(std::string{path.at(1)}) }; proto != ProtoRoutes.end() and path.at(2) == AppPath) {
		if (auto&& App{ Credentials->GetAppByKey(std::string{path.at(3)}) }; App) {
			return cwsserver::WsUpgrade(fd, path, headers);
		}
		HttpWriteResponse(fd, "403");
		fd.SocketClose();
		return -EACCES;
	}
	HttpWriteResponse(fd, "404");
	fd.SocketClose();
	return -ENOTDIR;
}

inet::socket_t cwebsocketserver::OnWsUpgrade(const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) {
	if (auto&& conn{ ProtoRoutes[std::string{path.at(1)}](Channels, Credentials->GetAppByKey(std::string{path.at(3)}), fd, path, headers) }; conn) {
		return conn;
	}
	else {
		HttpWriteResponse(fd, "400");
		fd.SocketClose();
	}
	return {};
}

cwebsocketserver::cwebsocketserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::server_t config) :
	inet::cwsserver{ "ws:srv", std::string{ config.Listen.hostport() }, config.Ssl.Context(), 8192 },
	Channels{ channels }, Credentials{ credentials }, AppPath{ config.Path }
{
	if (config.Proto & proto_t::type::websocket) {
		syslog.ob.print("Proto", "WebSocket ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Listen.hostport() }.c_str(), config.WebSocket.Path.c_str());
		ProtoRoutes[config.WebSocket.Path] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
			if (auto&& conn{ std::make_shared<cwssession>(channels, app, fd, config.WebSocket.MaxPayloadSize, config.WebSocket.PushOn,config.WebSocket.ActivityTimeout) }; conn and conn->OnWsConnect(path, headers)) {
				return std::dynamic_pointer_cast<inet::csocket>(conn);
			}
			return {};
		};
	}
	else {
		syslog.ob.print("Proto", "WebSocket ... disable");
	}

	if (config.Proto & proto_t::type::pusher) {
		syslog.ob.print("Proto", "Pusher ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Listen.hostport() }.c_str(), config.Pusher.Path.c_str());
		ProtoRoutes[config.Pusher.Path] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
			if (auto&& conn{ std::make_shared<cpushersession>(channels, app, fd, config.Pusher.MaxPayloadSize, config.Pusher.PushOn,config.Pusher.ActivityTimeout) }; conn and conn->OnWsConnect(path, headers)) {
				return std::dynamic_pointer_cast<inet::csocket>(conn);
			}
			return {};
		};
	}
	else {
		syslog.ob.print("Proto", "Pusher ... disable");
	}

	if (config.Proto & proto_t::type::mqtt3) {
		syslog.ob.print("Proto", "MQTT ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Listen.hostport() }.c_str(), config.MQTT.Path.c_str());
		ProtoRoutes[config.MQTT.Path] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
			return {};
		};
	}
	else {
		syslog.ob.print("Proto", "MQTT ... disable");
	}
}

cwebsocketserver::~cwebsocketserver() {

}
