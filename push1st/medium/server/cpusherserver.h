#pragma once
#include "../cserver.h"

namespace server {
	class cpusher : public cserver, public std::enable_shared_from_this<cpusher> {
	public:
		cpusher(const config_t& options);
		virtual ~cpusher();
	};
}