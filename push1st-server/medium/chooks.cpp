#include "chooks.h"
#include "../core/csyslog.h"
#include "../core/ci/casyncqueue.h"

static core::casyncqueue HookProcessing{ 2, "hookmgm" };

void cwebhook::Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, json::value_t&& msg)
{ 
	HookProcessing.enqueue([](std::weak_ptr<cwebhook> obj, hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t msg) {
		if (auto&& self{ obj.lock() }; self) {
			syslog.print(7, "[ WEBHOOK:%s ] %s %s#%s\n", std::string{ self->webEndpoint.hostport() }.c_str(), str(trigger), app.c_str(), channel.c_str());
			self->Write("POST", self->webEndpoint.url(), {
				{"Content-Type","application/json"},
				{"Connection", !self->fdKeepAlive ? "close" : "keep-alive"},
				{"Keep-Alive","30"} },
				json::serialize({
					{"event",str(trigger)},
					{"app",app},
					{"channel",channel},
					{"session",session},
					{"data", json::serialize(std::move(msg))},
			}));
		}
	}, weak_from_this(), trigger, app, channel, session, msg);
}


cwebhook::cwebhook(const std::string& endpoint, bool keepalive) : 
	webEndpoint{ endpoint }, fdKeepAlive{ keepalive }
{
	if (webEndpoint.ishttps()) {
		inet::SslCreateClientContext(fdSslCtx);
	}
}

inline void cwebhook::Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers, std::string&& request) {
	ssize_t res{ -ECONNRESET };
	std::unique_lock<decltype(fdLock)> lock(fdLock);
	if (Connect()) {
		if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, uri, std::move(headers), request); res == 0) {
			if (!fdKeepAlive) { inet::Close(fdEndpoint); }
			return;
		}
		if (inet::Close(fdEndpoint); Connect()) {
			if (res = HttpWriteRequest({ fdEndpoint, fdSsl }, method, uri, std::move(headers), request); res == 0) {
				if (!fdKeepAlive) { inet::Close(fdEndpoint); }
				return;
			}
		}
	}
	syslog.error("[ HOOK ] Connection %s lost ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
}

inline bool cwebhook::Connect() {
	if (fdEndpoint > 0 and inet::GetErrorNo(fdEndpoint) == 0) {
		return true;
	}
	if (fdEndpoint > 0) { fdSsl.reset(); inet::Close(fdEndpoint); }

	ssize_t res{ -1 };

	if (sockaddr_storage sa; (res = inet::GetSockAddr(sa, webEndpoint.hostport(), fdSsl ? "443" : "80", AF_INET)) == 0 ) {
		if (!fdSslCtx) {
			if ((res = inet::TcpConnect(fdEndpoint, sa, false, 800)) == 0) {
				//inet::SetTcpCork(fdEndpoint, false);
				inet::SetTcpNoDelay(fdEndpoint, true);
				::shutdown(fdEndpoint, SHUT_RD);
				return true;
			}
		}
		else if ((res = inet::SslConnect(fdEndpoint, sa, false, 800,fdSslCtx,fdSsl)) == 0) {
			::shutdown(fdEndpoint, SHUT_RD);
			return true;
		}
	}
	
	syslog.error("[ HOOK ] Connect %s error ( %s )\n", std::string{ webEndpoint.hostport() }.c_str(), std::strerror(-(int)res));
	return false;
}

void cluahook::Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, json::value_t&& msg) {
	if (luaAllowed) {
		HookProcessing.enqueue([](std::weak_ptr<cluahook> obj, hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t msg) {
			if (auto&& self{ obj.lock() }; self) {
				syslog.print(7, "[ LUAHOOK:%s ] %s %s#%s\n", self->luaScript.filename().c_str(), str(trigger), app.c_str(), channel.c_str());
				self->jitLua.luaExecute(self->luaScript, "OnTrigger", { str(trigger), app, channel, session, json::serialize(std::move(msg)) });
			}
		}, weak_from_this(), trigger, app, channel, session, msg);
	}
}