#pragma once
#include "../http/cwsserver.h"
#include "../../core/cconfig.h"

class cwspusherserver : public inet::cwsserver, public std::enable_shared_from_this<cwspusherserver>
{
public:
	cwspusherserver(const config::server_t& config);
	virtual ~cwspusherserver();
};

