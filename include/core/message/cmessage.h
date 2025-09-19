#pragma once
#include "core/util/cjson.h"
#include "core/util/medium.h"

namespace msg {
enum class delivery_t { broadcast = 1, multicast, unicast };
using object_t = std::shared_ptr<json::value_t>;

object_t Unserialize(const std::string_view& data, const std::string& producer);
inline object_t Unserialize(data_t&& data, const std::string& producer) {
    return Unserialize(ToString(data), producer);
}
object_t Make(json::value_t&& t, const std::string& producer);
}// namespace msg

using message_t = msg::object_t;