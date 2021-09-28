#pragma once
#include <jsoncpp/json/json.h>
#include <string>
#include <any>
#include <unordered_map>

class cjson {
public:
	using object_t = std::unordered_map<std::string, std::any>;
	using array_t = std::deque<std::any>;
	using value_t = Json::Value;
	static std::string serialize(object_t&& document);
	static bool unserialize(const std::string_view& document, value_t& value, std::string& err);
	static inline bool unserialize(const std::string_view& document, value_t& value) {
		std::string err; return unserialize(document, value, err);
	}
};

using json = cjson;