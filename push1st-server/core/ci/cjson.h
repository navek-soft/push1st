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
		return document.dump(-1);
	}
	static inline std::string serialize(const value_t& document) {
		return document.dump(-1);
	}
	static inline bool unserialize(const std::string_view& document, value_t& value) {
		try {
			value = std::move(value_t::parse(document.begin(), document.end()));
			return !value.empty();
		}
		catch (std::exception& ex) {

		}
		return false;
	}
};

using json = cjson;