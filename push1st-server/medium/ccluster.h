#pragma once
#include "medium.h"

class ccluster
{
public:
	ccluster(const config::cluster_t&) { ; }
	virtual ~ccluster() = default;
	virtual void Ping() { ; }
	virtual void Trigger(hook_t::type type, sid_t channel, sid_t session, data_t) { ; }
	virtual void Push(sid_t channel, sid_t session, data_t) { ; }
};