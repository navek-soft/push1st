#pragma once
#include "medium.h"
#include "../inet/csocket.h"
#include "../core/ci/cspinlock.h"
#include "../http/chttpconn.h"

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
};

class chook {
public:
	virtual ~chook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, data_t) = 0;
};

class cwebhook : public chook {
public:
	cwebhook(const std::shared_ptr<cconnection>& con, const std::string& endpoint) : webConnection{ con }, webEndpoint{ endpoint }{; }
	virtual ~cwebhook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, data_t) override;
private:
	std::shared_ptr<cconnection> webConnection;
	dsn_t webEndpoint;
};

class cluahook : public chook {
public:
	cluahook(const std::string& endpoint) { ; }
	virtual ~cluahook() { ; }
	virtual void Trigger(hook_t::type trigger, sid_t app, sid_t channel, sid_t session, data_t) override { ; }
};