#pragma once
#include "medium.h"
#include "../../lib/inet/csocket.h"
#include "../core/ci/cspinlock.h"
#include "../../lib/http/chttpconn.h"
#include "cmessage.h"
#include "../core/ci/cjson.h"
#include "../core/lua/clua.h"

/*
class cconnection : public inet::chttpconnection {
public:
	cconnection(const dsn_t& endpoint);
	void Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, std::string&& request = "");
private:
	inline bool Reconnect();
	fd_t fdEndpoint{ -1 };
	std::string fdHostPort;
	inet::ssl_ctx_t fdSslCtx;
	inet::ssl_t fdSsl;
	core::cspinlock fdLock;
};
*/

class chook {
public:
	virtual ~chook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, json::value_t&&) = 0;
};

class cwebhook : public chook, public inet::chttpconnection {
public:
	cwebhook(const std::string& endpoint, bool keepalive = false);
	virtual ~cwebhook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, json::value_t&&) override;
private:
	inline void Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, std::string&& request = "");
	inline bool Connect();
private:
	fd_t fdEndpoint{ -1 };
	inet::ssl_ctx_t fdSslCtx;
	inet::ssl_t fdSsl;
	dsn_t webEndpoint;
	bool fdKeepAlive{ false };
	core::cspinlock fdLock;
};

class cluahook : public chook {
public:
	cluahook(const std::string& endpoint) : luaAllowed{ std::filesystem::exists(endpoint) }, luaScript{ endpoint } {; }
	virtual ~cluahook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, json::value_t&&) override;
private:
	bool luaAllowed{ false };
	std::filesystem::path luaScript;
	clua::engine jitLua;
};