#pragma once
#include "../http/cwsserver.h"
#include "../core/cconfig.h"
#include "ccredentials.h"

class cchannels;

class cwebsocketserver : public inet::cwsserver, public std::enable_shared_from_this<cwebsocketserver>
{
public:
	cwebsocketserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::server_t config);
	virtual ~cwebsocketserver();
protected:
	virtual ssize_t WsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers);
	virtual inet::socket_t OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers) override;
	virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() override { return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this()); }
private:
	std::shared_ptr<cchannels> Channels;
	std::shared_ptr<ccredentials> Credentials;
	std::unordered_map<std::string, std::function<inet::socket_t(const std::shared_ptr<cchannels>&, const app_t&, const inet::csocket&, const http::path_t&, const http::params_t&, const http::headers_t&)>> ProtoRoutes;
	std::string AppPath{ "app" };
};

