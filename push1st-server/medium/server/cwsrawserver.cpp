#include "cwsrawserver.h"
#include "../../core/csyslog.h"

class cwsrawconnection : public http::cwebsocket, public inet::csocket, public std::enable_shared_from_this<cwsrawconnection>
{
public:
	cwsrawconnection(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers);
	virtual ~cwsrawconnection();
};

cwsrawconnection::cwsrawconnection(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers) : 
	inet::csocket{ std::move(fd) }
{
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cwsrawconnection::~cwsrawconnection() {
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

inet::socket_t cwsrawserver::OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) {
	return std::dynamic_pointer_cast<inet::csocket>(std::make_shared<cwsrawconnection>(fd,path,args, headers));
}

cwsrawserver::cwsrawserver(const config::server_t& config) :
	inet::cwsserver{ "ws:srv", std::string{ config.Listen.hostport() }, config.Ssl.Context(), 8192 }
{
	syslog.ob.print("websocket.raw", "...enable %s proto, listen on %s, path %s", config.Ssl.Enable ? "https" : "http", std::string{config.Listen.hostport()}.c_str(), config.Path.c_str());
}

cwsrawserver::~cwsrawserver() {

}
