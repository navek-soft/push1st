#include "../core/csyslog.h"
#include "capiserver.h"

capiserver::ctcpapiserver::ctcpapiserver(capiserver& api, config::interface_t config) :
	inet::chttpserver{ "ws:srv", std::string{ config.Tcp.hostport() }, config.Ssl.Context(), 8192 }, Api{ api }
{
	syslog.ob.print("Api", "Web ... enable %s proto, listen on %s, route /%s/", config.Ssl.Enable ? "https" : "http", std::string{ config.Tcp.hostport() }.c_str(), config.Path.c_str());
}

capiserver::ctcpapiserver::~ctcpapiserver() {

}

void capiserver::OnHttpRequest(const inet::csocket& fd, const std::string_view& method, const http::uri_t& path, const http::headers_t& headers, std::string&& request, std::string&& content) {
	printf("1");
}

void capiserver::Listen(const std::shared_ptr<inet::cpoll>& poll) 
{
	if (ApiTcp) { ApiTcp->Listen(poll); }
}


capiserver::capiserver(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccredentials>& credentials, config::interface_t config) :
	Channels{ channels }, Credentials{ credentials }, AppsPath{ config.Path }
{
	if (!config.Tcp.empty()) {
		ApiTcp = std::make_shared<ctcpapiserver>(*this, config);
	}
}

capiserver::~capiserver() {

}
