#pragma once
#include "core/config/cconfig.h"
#include "core/credentials/ccredentials.h"
#include "http/cwsserver.h"
#include "log/clog.h"

class cchannels;

class cwebsocketserver : public inet::cwsserver, public std::enable_shared_from_this<cwebsocketserver> {
    log_as(websocketserver);

   public:
    cwebsocketserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::server_t config);
    ~cwebsocketserver() override;

   protected:
    ssize_t WsUpgrade(const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) override;
    inet::socket_t OnWsUpgrade(const inet::socket_t& fd, const http::uri_t& path, const http::headers_t& headers) override;
    inline std::shared_ptr<inet::ctcpserver> TcpSelf() override {
        return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this());
    }

   private:
    std::shared_ptr<cchannels> Channels;
    std::shared_ptr<ccredentials> Credentials;
    std::unordered_map<std::string, std::function<inet::socket_t(const std::shared_ptr<cchannels>&, const app_t&, const inet::socket_t&, const http::uri_t&, const http::headers_t&)>> ProtoRoutes;
    std::string AppPath {"app"};
};
