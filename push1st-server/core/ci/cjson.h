#pragma once
#include "nlohmann/json.hpp"
//#include <jsoncpp/json/json.h>
#include <string>
#include <any>
#include <unordered_map>

class cjson {
public:
	using object_t = nlohmann::json::object_t;
	using array_t = nlohmann::json::array_t;
	using value_t = nlohmann::json;
	static inline std::string serialize(value_t&& document) {
		return document.dump(0);
	}
	static bool unserialize(const std::string_view& document, value_t& value, std::string& err) {
		value = std::move(value_t::parse(document.begin(), document.end()));
		return !value.empty();
	}
	static inline bool unserialize(const std::string_view& document, value_t& value) {
		std::string err; return unserialize(document, value, err);
	}
};

using json = cjson;