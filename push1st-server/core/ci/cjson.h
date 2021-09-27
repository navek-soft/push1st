#pragma once
#include <jsoncpp/json/json.h>
#include <any>
#include <string>
#include <memory>

class cjson {
public:
	using value_t = Json::Value;
	static value_t unserialize(const std::string_view& document);
};

using json = cjson;