#include "chooks.h"
#include "../core/csyslog.h"
#include "../core/ci/cjson.h"

void cwebhook::Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, data_t msg)
{ 
	webConnection->Write("POST", "/webhook/", {
			{"Content-Type","application/json"},
			{"Connection","keep-alive"},
			{"Keep-Alive","30"} }, 
			json::serialize({
				{"event",str(trigger)},
				{"app",app},
				{"channel",channel},
				{"session",session},
				{"data",msg.first? std::string_view {(const char*)msg.first.get(),msg.second} : std::string_view{}},
			}));
}

cconnection::cconnection(const dsn_t& endpoint) : 
	fdHostPort{ endpoint.hostport() }
{
	if (endpoint.ishttps()) {
		inet::SslCreateClientContext(fdSslCtx);
	}
}

void cconnection::Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers, std::string&& request) {
	ssize_t res{ -ECONNRESET };
	if (Reconnect()) {
		if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, uri, std::move(headers), request); res == 0) {
			return;
		}
		else if (Reconnect()) {
			if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, uri, std::move(headers), request); res == 0) {
				return;
			}
		}
	}
	syslog.error("[ HOOK ] Connection %s lost ( %s )\n", fdHostPort.c_str(), std::strerror(-(int)res));
}

inline bool cconnection::Reconnect() {
	if (fdEndpoint > 0 and inet::GetErrorNo(fdEndpoint) == 0) {
		return true;
	}
	if (fdEndpoint > 0) { fdSsl.reset(); inet::Close(fdEndpoint); }

	ssize_t res{ -1 };

	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, fdHostPort, fdSsl ? "443" : "80", AF_INET)) == 0 ) {
		if (!fdSslCtx) {
			if ((res = inet::TcpConnect(fdEndpoint, sa, false, 2500)) == 0) {
				::shutdown(fdEndpoint, SHUT_RD);
				return true;
			}
		}
		else if ((res = inet::SslConnect(fdEndpoint, sa, false, 2500,fdSslCtx,fdSsl)) == 0) {
			::shutdown(fdEndpoint, SHUT_RD);
			return true;
		}
	}

	
	syslog.error("[ HOOK ] Connect %s error ( %s )\n", fdHostPort.c_str(), std::strerror(-(int)res));
	return false;
}