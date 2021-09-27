#pragma once
#include <chrono>
#include "medium.h"

enum class delivery_t { broadcast = 1, multicast, unicast };

class cmessage
{
public:
	cmessage(data_t&& msg, sid_t event, sid_t channel, sid_t producer, delivery_t delivery, std::time_t ttl);
	virtual ~cmessage();
	inline std::string_view GetData() { return std::string_view{ (char*)Data.first.get(), Data.second }; }
public:
	uint64_t Id;
	int64_t TimeArrival, TimeExpireAt{ 0 };
	std::string Event, Channel, Producer;
	delivery_t Delivery{ delivery_t::multicast };
	std::unordered_set<std::string> To;
	array_t Data;
};