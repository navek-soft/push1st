#include "chooks.h"
#include "../core/csyslog.h"
#include "../core/ci/casyncqueue.h"

static core::casyncqueue HookProcessing{ 2, "hookmgm" };

void cwebhook::Trigger(hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t&& msg)
{ 
	syslog.print(7, "[ WEBHOOK:%s ] %s %s#%s\n", std::string{ webEndpoint.hostport() }.c_str(), str(trigger), app.c_str(), channel.c_str());
	std::string request{ std::move(json::serialize({
			{"event",str(trigger)},
			{"app",app},
			{"channel",channel},
			{"session",session},
			{"data", json::serialize(std::move(msg))},
		})) };
	HookProcessing.enqueue([this, request]() {
		Write("POST", webEndpoint.url(), {
			{"Accept","application/json"},
			{"Content-Type","application/json"},
			{"Connection", !fdKeepAlive ? "close" : "keep-alive"},
			{"Host", std::string{ webEndpoint.host()} } }, request);
	});
}


cwebhook::cwebhook(const std::string& endpoint, bool keepalive) : 
	webEndpoint{ endpoint }, fdKeepAlive{ keepalive }
{
	if (webEndpoint.ishttps()) {
		inet::SslCreateClientContext(fdSslCtx);
	}
}

inline void cwebhook::ReadResponse() {
	std::string_view method,content;
	http::uri_t path;
	http::headers_t headers;
	std::string request;
	if (HttpReadRequest({ fdEndpoint, fdSsl }, method, path, headers, request, content, 65536) != 0) {
		inet::Close(fdEndpoint);
		fdSsl.reset();
	}
}

void cwebhook::Push(const std::string& trigger, const std::string& channel, std::string method, json::value_t&& data, std::unordered_map<std::string_view, std::string> headers) {
	headers.emplace("Accept", "application/json");
	headers.emplace("Content-Type", "application/json");
	headers.emplace("Connection", "close");
	headers.emplace("Host", std::string{ webEndpoint.host() });

	syslog.print(7, "[ WEBHOOK:%s ] %s:%s\n", std::string{ webEndpoint.hostport() }.c_str(), channel.c_str(), trigger.c_str());

	HookProcessing.enqueue([this, method, request = json::serialize(std::move(data)), headers]() {
		Write(method, webEndpoint.url(), std::move(std::unordered_map<std::string_view, std::string>{headers}), std::string{ request });
	});
}

void cwebhook::Send(const std::string_view& method, json::value_t&& data, std::unordered_map<std::string_view, std::string>&& headers) {
	ssize_t res{ -ECONNRESET };
	headers["Host"] = std::string{ webEndpoint.host() };
	std::unique_lock<decltype(fdLock)> lock(fdLock);
	if (Connect()) {
		if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, webEndpoint.url(), std::move(headers), json::serialize(std::move(data))); res == 0) {
			ReadResponse();
		}
		else {
			syslog.error("[ HOOK ] Send event %s error ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
		}

		inet::Close(fdEndpoint);
		fdSsl.reset();
		fdEndpoint = -1;
		return;
	}
	syslog.error("[ HOOK ] Connection %s lost ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
}

inline void cwebhook::Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers, const std::string& request) {
	ssize_t res{ -ECONNRESET };
	std::unique_lock<decltype(fdLock)> lock(fdLock);
	if (Connect()) {
		if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, uri, std::move(headers), request); res == 0) {
			ReadResponse();
		}
		else {
			syslog.error("[ HOOK ] Send event %s error ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
		}

		inet::Close(fdEndpoint); 
		fdSsl.reset(); 
		fdEndpoint = -1;
		return;
	}
	syslog.error("[ HOOK ] Connection %s lost ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
}

inline bool cwebhook::Connect() {
	/*
	if (fdEndpoint > 0 and inet::GetErrorNo(fdEndpoint) == 0) {
		return true;
	}
	*/

	fdSsl.reset(); inet::Close(fdEndpoint); fdEndpoint = -1;
	
	ssize_t res{ -1 };

	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, webEndpoint.hostport(), fdSsl ? "443" : "80", AF_INET)) == 0 ) {
		if (!fdSslCtx) {
			if ((res = inet::TcpConnect(fdEndpoint, sa, false, 1500)) == 0) {
				//inet::SetRecvTimeout(fdEndpoint, 1000);
				//inet::SetTcpCork(fdEndpoint, false);
				//inet::SetTcpNoDelay(fdEndpoint, true);
				//::shutdown(fdEndpoint, SHUT_RD);
				return true;
			}
		}
		else if ((res = inet::SslConnect(fdEndpoint, sa, false, 1500,fdSslCtx,fdSsl)) == 0) {
			return true;
		}
		fdEndpoint = -1;
	}
	
	syslog.error("[ HOOK ] Connect %s error ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
	return false;
}

void cluahook::Trigger(hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t&& msg) {
	if (luaAllowed) {
		std::string data{ json::serialize(std::move(msg)) };
		HookProcessing.enqueue([this, trigger, app, channel, session, data]() {
			jitLua.luaExecute(luaScript, "OnTrigger", { str(trigger), app, channel, session, data });
		});
	}
}