#include "core/message/cmessage.h"

#include <atomic>
#include <chrono>

#include "log/clog.h"

static std::atomic_uint64_t MsgId {(uint64_t)std::chrono::system_clock::now().time_since_epoch().count()};

message_t msg::Make(json::value_t&& t, const std::string& producer) {
    message_t msg {new json::value_t {std::move(t)}};
    (*msg)["#msg-id"] = ++MsgId;
    (*msg)["#msg-arrival"] = std::chrono::system_clock::now().time_since_epoch().count();
    (*msg)["#msg-from"] = producer;
    (*msg)["#msg-delivery"] = "broadcast";
    (*msg)["#msg-expired"] = 0;
    return msg;
}

message_t msg::Unserialize(const std::string_view& data, const std::string& producer) {
    try {
        auto&& msg {std::make_shared<json::value_t>()};
        if (msg and json::Unserialize(data, *msg)) {
            (*msg)["#msg-id"] = ++MsgId;
            (*msg)["#msg-arrival"] = std::chrono::system_clock::now().time_since_epoch().count();
            (*msg)["#msg-from"] = producer;
            (*msg)["#msg-expired"] = 0;
            (*msg)["#msg-delivery"] = "broadcast";
            if ((*msg).contains("options")) {
                if ((*msg)["options"].contains("ttl")) {
                    (*msg)["#msg-expired"] = (std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den)
                                             + (*msg)["options"]["ttl"].get<size_t>();
                }
                if ((*msg)["options"].contains("delivery")) {
                    if (auto&& val {(*msg)["options"]["delivery"].get<std::string_view>()}; val == "multicast") {
                        (*msg)["#msg-delivery"] = "multicast";
                    } else if (val == "unicast") {
                        (*msg)["#msg-delivery"] = "unicast";
                    }
                }
                if ((*msg)["options"].contains("session")) {
                    (*msg)["socket_id"] = (*msg)["options"]["session"].get<std::string>();
                }
            }

            return msg;
        }
    } catch (std::exception& ex) { PSHT_DEFAULT_ERROR("Message data ( {} )", ex.what()); }
    return {};
}

/*
cmessage::cmessage(data_t&& msg, sid_t event, sid_t channel, sid_t producer, delivery_t delivery, std::time_t ttl) :
    Id{ ++MsgId },
    TimeArrival{ std::chrono::system_clock::now().time_since_epoch().count() },
    TimeExpireAt{ !ttl ? 0 : std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + ttl },
    Event{ event }, Channel{ channel }, Producer{ producer }, Delivery{ delivery }, Data{ std::move(msg) }
{
    ;
}

cmessage::~cmessage() {
    ;
}
*/