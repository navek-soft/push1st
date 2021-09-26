#pragma once
#include "../http/cwsserver.h"
#include "../http/cwebsocket.h"
#include "../../core/cconfig.h"

class cwsrawserver : public inet::cwsserver, public std::enable_shared_from_this<cwsrawserver>
{
public:
	cwsrawserver(const config::server_t& config);
	virtual ~cwsrawserver();
protected:
	virtual inet::socket_t OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) override;
	virtual inline std::shared_ptr<inet::ctcpserver> TcpSelf() override { return std::dynamic_pointer_cast<inet::ctcpserver>(shared_from_this()); }
};

