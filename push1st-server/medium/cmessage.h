#pragma once
#include "medium.h"
#include "../core/ci/cjson.h"

namespace msg {
	enum class delivery_t { broadcast = 1, multicast, unicast };
	using object_t = std::shared_ptr<json::value_t>;

	object_t unserialize(const std::string_view& data, const std::string& producer);
	inline object_t unserialize(data_t&& data, const std::string& producer) { return unserialize(to_string(data), producer); }
	object_t make(json::value_t&& t, const std::string& producer);
}

using message_t = msg::object_t;