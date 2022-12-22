#pragma once
#include "medium.h"
#include "../../lib/inet/csocket.h"
#include "../core/ci/cspinlock.h"
#include "../../lib/http/chttpconn.h"
#include "cmessage.h"
#include "../core/ci/cjson.h"
#include "../core/lua/clua.h"

class chook {
public:
	virtual ~chook() { ; }
	virtual void Trigger(hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t&&) = 0;
};

class cwebhook : public chook, public inet::chttpconnection, public std::enable_shared_from_this<cwebhook> {
public:
	cwebhook(const std::string& endpoint, bool keepalive = false);
	virtual ~cwebhook() { ; }
	virtual void Trigger(hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t&&) override;
	void Push(const std::string& trigger, const std::string& channel, std::string method, json::value_t&& data, std::unordered_map<std::string_view, std::string> headers);
private:
	void Send(const std::string_view& method, json::value_t&& data, std::unordered_map<std::string_view, std::string>&& headers = {});
	inline void Write(const std::string_view& method, const std::string_view& uri, std::unordered_map<std::string_view, std::string>&& headers = {}, const std::string& request = "");
	inline bool Connect();
	inline void ReadResponse();
private:
	fd_t fdEndpoint{ -1 };
	inet::ssl_ctx_t fdSslCtx;
	inet::ssl_t fdSsl;
	dsn_t webEndpoint;
	bool fdKeepAlive{ false };
	core::cspinlock fdLock;
};

class cluahook : public chook, public std::enable_shared_from_this<cluahook> {
public:
	cluahook(const std::string& endpoint) : luaAllowed{ std::filesystem::exists(endpoint) }, luaScript{ endpoint } {; }
	virtual ~cluahook() { ; }
	virtual void Trigger(hook_t::type trigger, std::string app, std::string channel, std::string session, json::value_t&&) override;
private:
	bool luaAllowed{ false };
	std::filesystem::path luaScript;
	clua::engine jitLua;
};