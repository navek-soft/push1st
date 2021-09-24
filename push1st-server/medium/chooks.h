#pragma once
#include "medium.h"

class chook {
public:
	virtual ~chook() { ; }
	virtual void Trigger(sid_t channel, sid_t session, data_t) = 0;
};

class cwebhook : public chook {
public:
	cwebhook(const std::string& endpoint) { ; }
	virtual ~cwebhook() { ; }
	virtual void Trigger(sid_t channel, sid_t session, data_t) override { ; }
};

class cluahook : public chook {
public:
	cluahook(const std::string& endpoint) { ; }
	virtual ~cluahook() { ; }
	virtual void Trigger(sid_t channel, sid_t session, data_t) override { ; }
};