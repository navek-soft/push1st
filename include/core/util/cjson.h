#pragma once
#include "nlohmann/json.hpp"
// #include <jsoncpp/json/json.h>
#include <any>
#include <string>
#include <unordered_map>

class cjson {
   public:
    using object_t = nlohmann::json::object_t;
    using array_t = nlohmann::json::array_t;
    using value_t = nlohmann::json;
    static inline std::string Serialize(value_t&& document) {
        return document.dump(-1);
    }
    static inline std::string Serialize(const value_t& document) {
        return document.dump(-1);
    }
    static inline bool Unserialize(const std::string_view& document, value_t& value) {
        try {
            value = value_t::parse(document.begin(), document.end());
            return !value.empty();
        } catch (std::exception& ex) {}
        return false;
    }
};

using json = cjson;