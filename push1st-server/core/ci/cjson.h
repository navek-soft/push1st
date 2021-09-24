#pragma once
#include <jsoncpp/json/value.h>

using json_t = Json::Value;

namespace json {
	static inline auto unserialize(const std::string& value) { return json_t(); }
	static inline std::string serialize(const json_t& value) { return {}; }
}