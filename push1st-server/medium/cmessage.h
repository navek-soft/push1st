#pragma once
#include "medium.h"

#include "../core/ci/cjson.h"

namespace msg {
	enum class delivery_t { broadcast = 1, multicast, unicast };
	using object_t = std::shared_ptr<json::value_t>;

	object_t unserialize(const std::string_view& data, const std::string& producer);
	inline object_t unserialize(data_t&& data, const std::string& producer) { return unserialize(to_string(data), producer); }
	inline json::value_t& ref(const object_t& obj) { return *(obj.get()); }
}

using message_t = msg::object_t;

/*
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
*/