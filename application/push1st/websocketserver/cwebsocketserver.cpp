#include "cwebsocketserver.h"

#include "core/credentials/ccredentials.h"
#include "log/clog.h"

#include "../proto/cpushersession.h"
#include "../proto/cwssession.h"

ssize_t cwebsocketserver::WsUpgrade(const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) {
    if (auto&& proto {ProtoRoutes.find(std::string {path.At(1)})}; proto != ProtoRoutes.end()) {
        if (path.At(1) != AppPath) {
            if (auto&& App {Credentials->GetAppByKey(std::string {path.At(3)})}; App) {
                return cwsserver::WsUpgrade(fd, path, headers);
            }
        } else if (path.At(1) == AppPath /* Defaul protocol */) {
            if (auto&& App {Credentials->GetAppByKey(std::string {path.At(2)})}; App) {
                return cwsserver::WsUpgrade(fd, path, headers);
            }
        }

        HttpWriteResponse(*fd, "403");
        fd->SocketClose();
        return -EACCES;
    }
    HttpWriteResponse(*fd, "404");
    fd->SocketClose();
    return -ENOTDIR;
}

inet::socket_t cwebsocketserver::OnWsUpgrade(const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) {
    std::string AppId {std::string {path.At(path.At(1) != AppPath ? 3 : 2)}};
    auto&& App {Credentials->GetAppByKey(AppId)};
    if (auto&& conn {ProtoRoutes[std::string {path.At(1)}](Channels, App, fd, path, headers)}; App and conn) {
        return conn;
    } else {
        HttpWriteResponse(*fd, "400");
        fd->SocketClose();
    }
    return {};
}

cwebsocketserver::cwebsocketserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::server_t config) :
    inet::cwsserver {"ws:srv", config.Threads, config.Accept, std::string {config.Listen.HostPort()}, config.Ssl.Context(), 8192},
    Channels {channels},
    Credentials {credentials},
    AppPath {config.Path} {
    if (config.Proto & proto_t::type::websocket) {
        PSHT_INFO("WebSocket (Proto) ... enable {} proto, listen on {}, route /{}/",
                  config.Ssl.Enable ? "https" : "http",
                  std::string {config.Listen.HostPort()}.c_str(),
                  config.WebSocket.Path.c_str());
        ProtoRoutes[config.WebSocket.Path] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
            if (auto&& conn {std::make_shared<cwssession>(
                    channels, app, *fd, config.WebSocket.MaxPayloadSize, config.WebSocket.PushOn, config.WebSocket.ActivityTimeout, std::string {path.Arg("session")})};
                conn and conn->OnWsConnect(path, headers)) {
                return std::dynamic_pointer_cast<inet::csocket>(conn);
            }
            return {};
        };
    } else {
        PSHT_INFO("WebSocket (Proto) ... disable");
    }

    if (config.Proto & proto_t::type::pusher) {
        /* Set Pusher as default proto /app/<app-key> */
        PSHT_INFO("Default (Proto) ... Pusher {} proto, listen on {}, route /{}/",
                  config.Ssl.Enable ? "https" : "http",
                  std::string {config.Listen.HostPort()}.c_str(),
                  "app");
        ProtoRoutes["app"] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
            if (auto&& conn {std::make_shared<cpushersession>(
                    channels, app, *fd, config.Pusher.MaxPayloadSize, config.Pusher.PushOn, config.Pusher.ActivityTimeout)};
                conn and conn->OnWsConnect(path, headers)) {
                return std::dynamic_pointer_cast<inet::csocket>(conn);
            }
            return {};
        };
        PSHT_INFO("Pusher (Proto) ... enable {} proto, listen on {}, route /{}/",
                  config.Ssl.Enable ? "https" : "http",
                  std::string {config.Listen.HostPort()}.c_str(),
                  config.Pusher.Path.c_str());
        ProtoRoutes[config.Pusher.Path] = [config](const std::shared_ptr<cchannels>& channels, const app_t& app, const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) -> inet::socket_t {
            if (auto&& conn {std::make_shared<cpushersession>(
                    channels, app, *fd, config.Pusher.MaxPayloadSize, config.Pusher.PushOn, config.Pusher.ActivityTimeout)};
                conn and conn->OnWsConnect(path, headers)) {
                return std::dynamic_pointer_cast<inet::csocket>(conn);
            }
            return {};
        };
    } else {
        PSHT_INFO("Pusher (Proto) ... disable");
    }

    if (config.Proto & proto_t::type::mqtt3) {
        PSHT_INFO("MQTT (Proto) ... enable {} proto, listen on {}, route /{}/",
                  config.Ssl.Enable ? "https" : "http",
                  std::string {config.Listen.HostPort()}.c_str(),
                  config.MQTT.Path.c_str());
        ProtoRoutes[config.MQTT.Path] = [config]([[maybe_unused]] const std::shared_ptr<cchannels>& channels, [[maybe_unused]] const app_t& app, [[maybe_unused]] const inet::socket_t& fd, [[maybe_unused]] const http::uri_t& path, [[maybe_unused]] const http::headers_t& headers) -> inet::socket_t {
            return {};
        };
    } else {
        PSHT_INFO("MQTT (Proto) ... disable");
    }
}

cwebsocketserver::~cwebsocketserver() {}
