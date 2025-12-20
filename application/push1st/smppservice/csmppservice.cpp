#include "csmppservice.h"

#include <netinet/in.h>
#include <unistd.h>

#include <cinttypes>
#include <cstddef>
#include <cstdint>

#include "log/clog.h"

namespace smpp {
enum class receipt_t : uint8_t { none = 0, always = 1, failure = 2, success = 3 };

enum class bind_t : uint32_t { none = 0, transmitter = 2, receiver = 1, transceiver = 0x009 };

std::string Utf8ToUtf16(const std::string& utf8) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string utf16 = convert.from_bytes(utf8);

    for (auto& ch : utf16) {
        ch = htobe16(ch);
    }

    return {(char*)utf16.data(), utf16.size() * 2};
}

template <size_t __begin, size_t... __indices, typename Tuple>// NOLINT
auto TupleSliceImpl(Tuple&& tuple, std::index_sequence<__indices...>) {
    return std::make_tuple(std::get<__begin + __indices>(std::forward<Tuple>(tuple))...);
}

template <size_t __begin, size_t __count, typename Tuple>// NOLINT
auto TupleSlice(Tuple&& tuple) {
    static_assert(__count > 0, "splicing tuple to 0-length is weird...");
    return TupleSliceImpl<__begin>(std::forward<Tuple>(tuple), std::make_index_sequence<__count>());
}

template <size_t __begin, size_t __count, typename Tuple>// NOLINT
using tuple_slice_t = decltype(tuple_slice<__begin, __count>(Tuple {}));

class cpacker {
   public:
    cpacker(size_t reserved) {
        data.reserve(reserved);
    }
    inline cpacker& Write(uint8_t v) {
        data.push_back(v);
        offset += sizeof(v);
        return *this;
    }
    inline cpacker& Write(uint32_t v) {
        v = htobe32(v);
        data.append((char*)&v, sizeof(v));
        offset += sizeof(v);
        return *this;
    }
    inline cpacker& Write(uint16_t v) {
        v = htobe16(v);
        data.append((char*)&v, sizeof(v));
        offset += sizeof(v);
        return *this;
    }
    inline cpacker& Write(const std::string& v) {
        data.append(v.data(), v.length()).push_back('\0');
        offset += v.length() + 1;
        return *this;
    }
    inline cpacker& Write(const void* v, size_t len) {
        data.append((char*)v, len);
        offset += len;
        return *this;
    }
    inline std::string Str() {
        data.shrink_to_fit();
        return data;
    }

   private:
    size_t offset {0};
    std::string data;
};
namespace param {
#pragma pack(push, 1)
    class ccmd {
        uint32_t len {0};

       public:
        ccmd() = default;
        ccmd(uint32_t _id, uint32_t _status, uint32_t _seq) :
            len {sizeof(ccmd)},
            id {_id},
            status {_status},
            sequence {_seq} {
            ;
        }
        inline void operator()(uint32_t _id, uint32_t _status, uint32_t _seq) {
            id = _id;
            status = _status;
            sequence = _seq;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(len).Write(id).Write(status).Write(sequence);
            return package;
        }
        inline void Length(uint32_t _len) {
            len = htobe32(_len);
        }
        inline uint32_t Length() {// NOLINT (static)
            return be32toh(len);
        }
        inline void Unpack(std::string_view& data) {
            len = be32toh(*(uint32_t*)data.data());
            data.remove_prefix(4);
            id = be32toh(*(uint32_t*)data.data());
            data.remove_prefix(4);
            status = be32toh(*(uint32_t*)data.data());
            data.remove_prefix(4);
            sequence = be32toh(*(uint32_t*)data.data());
            data.remove_prefix(4);
        }

       public:
        uint32_t id {0}, status {0}, sequence {0};
    };
#pragma pack(pop)
    class cservice {
        std::string type;

       public:
        cservice() = default;
        inline void operator()(const std::string& id) {
            type = id;
        }
        inline cpacker& Pack(cpacker& package) {
            package.Write(type);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            type.assign(data.data());
            data.remove_prefix(type.size() + 1);
        }
    };
    class caddress {
        uint8_t ton {0}, npi {0};
        std::string address;

       public:
        caddress() = default;
        inline caddress(uint8_t _ton, uint8_t _npi, const std::string& _addr) {
            ton = _ton;
            npi = _npi;
            address = _addr;
        }
        inline void operator()(uint8_t _ton, uint8_t _npi, const std::string& _addr) {
            ton = _ton;
            npi = _npi;
            address = _addr;
        }
        inline cpacker& Pack(cpacker& package) {
            package.Write(ton).Write(npi).Write(address);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            ton = (uint8_t)data.front();
            data.remove_prefix(1);
            npi = (uint8_t)data.front();
            data.remove_prefix(1);
            address.assign(data.data());
            data.remove_prefix(address.size() + 1);
        }
    };
    class cesm {
        uint8_t value {0};

       public:
        cesm() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline void Receipt(receipt_t type) {
            value = (value & 0xfc) | (uint8_t)type;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };
    class cprotocol {
        uint8_t value {0};

       public:
        cprotocol() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };
    class cpriority {
        uint8_t value {0};

       public:
        cpriority() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };
    class cscheduledeliverytime {
        std::string value;

       public:
        cscheduledeliverytime() = default;
        inline void operator()(const std::string& val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value.assign(data.data());
            data.remove_prefix(value.size() + 1);
        }
    };
    class cvalidityperiod {
        std::string value;

       public:
        cvalidityperiod() = default;
        inline void operator()(const std::string& val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value.assign(data.data());
            data.remove_prefix(value.size() + 1);
        }
    };
    class cregistereddelivery {
        uint8_t value {0};

       public:
        cregistereddelivery() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
    };
    class creplaceifpresent {
        uint8_t value {0};

       public:
        creplaceifpresent() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };

    class cencoding {
        uint8_t value {0};

       public:
        enum cp { smsc = 0, latin1 = 3, ucs2 = 8, cyrillic = 6 };
        cencoding() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline std::string Encode(const std::string& msg) const {
            return value != cencoding::cp::ucs2 ? msg : Utf8ToUtf16(msg);
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };

    class cudh {
       public:
        uint8_t len {5};
        uint16_t tag {0x3};
        uint8_t id {1}, parts {0}, no {0};
        inline void operator()(uint8_t nparts, uint8_t nno) {
            parts = nparts;
            no = nno;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            return package.Write(len).Write(tag).Write(id).Write(parts).Write(no);
        }
    };

    class csmdefaultmsgid {
        uint8_t value {0};

       public:
        csmdefaultmsgid() = default;
        inline void operator()(uint8_t val) {
            value = val;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = (uint8_t)data.front();
            data.remove_prefix(1);
        }
    };

    class cmessage {
        std::string value;

       public:
        cmessage() = default;
        inline void operator()(const std::string& msg) {
            value = msg;
        }
        inline cpacker& Pack(cpacker& package) {
            package.Write((uint8_t)value.length()).Write(value.data(), value.length());
            return package;
        }
        inline void Unpack(std::string_view& data) {
            size_t len = (uint8_t)data.front();
            data.remove_prefix(1);
            value.assign(data.data(), len);
            data.remove_prefix(len);
        }
        inline std::vector<std::string> Split(const std::string& msg) {// NOLINT (static)
            if (msg.length() <= 140) {                                 /* no split 140 symbols */
                return {msg};
            }
            std::vector<std::string> parts;
            size_t nparts {(msg.length() + 131) / 132};
            parts.reserve(nparts);
            std::string_view msgpart {msg};
            param::cudh udh;
            for (size_t npart {1}; !msgpart.empty(); ++npart) {
                cpacker package(144);
                udh((uint8_t)nparts, (uint8_t)npart);
                auto len {std::min(132UL, msgpart.length())};
                udh.Pack(package).Write(msgpart.data(), len);
                msgpart.remove_prefix(len);
                parts.emplace_back(package.Str());
            }
            return parts;
        }
        const std::string& Str() {
            return value;
        }
    };

    class cstring {
        std::string value;

       public:
        cstring(const std::string& val = {}) : value {val} {
            ;
        }
        inline void operator()(const std::string& msg) {
            value = msg;
        }
        inline cpacker& Pack(cpacker& package) {
            package.Write(value);
            return package;
        }
        inline void Unpack(std::string_view& data) {
            value = std::string {data.data()};
            data.remove_prefix(value.size() + 1);
        }
        inline auto Str() const {
            return value;
        }
    };

    class cinterface {
        uint8_t value {0};

       public:
        cinterface(uint8_t val = 0) : value {val} {
            ;
        }
        inline void operator()(uint8_t ver) {
            value = ver;
        }
        inline cpacker& Pack(cpacker& package) {// NOLINT (static)
            package.Write(value);
            return package;
        }
    };

}// namespace param

class cenquire {
   public:
    static inline const uint32_t req_id {0x00000015};
    static inline const uint32_t resp_id {0x80000015};
    param::ccmd command;
    cenquire(uint32_t seq, uint32_t id = resp_id, uint32_t status = 0) : command {id, status, seq} {
        ;
    }

    std::string Pack() {
        cpacker packer {512};
        command.Pack(packer);
        auto&& msg {packer.Str()};
        ((param::ccmd*)msg.data())->Length((uint32_t)msg.length());
        return msg;
    }
};

class csms {
   public:
    static inline const uint32_t req_id {4};
    static inline const uint32_t resp_id {0x80000004};
    csms(uint32_t seq, uint32_t id = req_id, uint32_t status = 0) : command {id, status, seq} {
        ;
    }
    param::ccmd command;
    param::cservice service;
    param::caddress src, dst;
    param::cesm esm;
    param::cprotocol proto;
    param::cpriority priority;
    param::cscheduledeliverytime delivery_time;
    param::cvalidityperiod valid_period;
    param::cregistereddelivery register_delivery;
    param::creplaceifpresent replace;
    param::cencoding encoding;
    param::csmdefaultmsgid msg_id;
    param::cmessage message;
    std::string Pack() {
        cpacker packer {512};
        command.Pack(packer);
        service.Pack(packer);
        src.Pack(packer);
        dst.Pack(packer);
        esm.Pack(packer);
        proto.Pack(packer);
        priority.Pack(packer);
        delivery_time.Pack(packer);
        valid_period.Pack(packer);
        register_delivery.Pack(packer);
        replace.Pack(packer);
        encoding.Pack(packer);
        msg_id.Pack(packer);
        message.Pack(packer);

        auto&& msg {packer.Str()};
        ((param::ccmd*)msg.data())->Length((uint32_t)msg.length());
        return msg;
    }
};

class cbind {
   public:
    cbind(uint32_t seq, bind_t id = bind_t::transceiver, uint32_t status = 0) : command {(uint32_t)id, status, seq} {
        ;
    }
    param::ccmd command;
    param::cstring login, password, system {};
    param::cinterface version {0x34};
    param::caddress addr {0, 0, {}};
    std::string Pack() {
        cpacker packer {512};
        command.Pack(packer);
        login.Pack(packer);
        password.Pack(packer);
        system.Pack(packer);
        version.Pack(packer);
        addr.Pack(packer);

        auto&& msg {packer.Str()};
        ((param::ccmd*)msg.data())->Length((uint32_t)msg.length());
        return msg;
    }
};

class cunbind {};

class cdelivery_resp {
   public:
    static inline const uint32_t id {0x80000005};
    cdelivery_resp(uint32_t seq, const std::string& msg, uint32_t status = 0) :
        command {(uint32_t)id, status, seq},
        msg_id {msg} {
        ;
    }

    std::string Pack() {
        cpacker packer {512};
        command.Pack(packer);
        msg_id.Pack(packer);
        auto&& msg {packer.Str()};
        ((param::ccmd*)msg.data())->Length((uint32_t)msg.length());
        return msg;
    }

   private:
    param::ccmd command;
    param::cstring msg_id;
};

class cdelivery {
   public:
    static inline const uint32_t id {5};
    param::cservice service;
    param::caddress src, dst;
    param::cesm esm;
    param::cprotocol proto;
    param::cpriority priority;
    param::cscheduledeliverytime delivery_time;
    param::cvalidityperiod valid_period;
    param::cregistereddelivery register_delivery;
    param::creplaceifpresent replace;
    param::cencoding encoding;
    param::cstring predefined;
    param::cmessage message;

    inline void Unpack(std::string_view& data) {
        service.Unpack(data);
        src.Unpack(data);
        dst.Unpack(data);
        esm.Unpack(data);
        proto.Unpack(data);
        priority.Unpack(data);
        delivery_time.Unpack(data);
        valid_period.Unpack(data);
        register_delivery.Unpack(data);
        replace.Unpack(data);
        encoding.Unpack(data);
        predefined.Unpack(data);
        message.Unpack(data);
    }
};

template <typename... ARGS>
class cresponse {
    using response_t = std::tuple<ARGS...>;
    template <typename T>
    inline void Unpack(std::string_view& data, T& val) {
        val.Unpack(data);
    }
    inline void Unpack([[maybe_unused]] std::string_view& data) {
        ;
    }

   public:
    inline response_t operator()(std::string_view& data) {
        auto&& response {std::tuple_cat(std::tuple<std::string_view&>(data), response_t {})};
        std::apply(
            [](std::string_view& data, auto&&... xs) {
                ((std::forward<decltype(xs)>(xs).Unpack(data)), ...);
            },
            response);
        return TupleSlice<1, std::tuple_size<response_t>::value>(response);// NOLINT (need fix)
        // return {};
    }
};
}// namespace smpp

void csmppservice::ReleaseGateway(const std::string& gwLogin, const std::string& gwPassword) {
    std::string conId {gwLogin + ":" + gwPassword};
    size_t removed {0};
    {
        std::unique_lock lock(gwConnectionLock);
        removed = gwConnections.erase(conId);
    }

    if (removed) {
        PSHT_DEBUG("Release {} gateway", conId);
    }
}

std::pair<std::string, json::value_t> csmppservice::Send(const json::value_t& message) {
    try {
        std::shared_ptr<cgateway> con;
        if (std::string gwLogin {message.contains("login") ? message["login"].get<std::string>() : std::string {}};
            !gwLogin.empty()) {
            std::string gwPassword {message.contains("password") ? message["password"].get<std::string>() : std::string {}};
            std::vector<std::string> gwHosts;

            if (message.contains("hosts")) {
                if (message["hosts"].is_array()) {
                    for (auto&& host : message["hosts"]) {
                        gwHosts.emplace_back(host.get<std::string>());
                    }
                } else if (message["hosts"].is_string()) {
                    gwHosts.emplace_back(message["hosts"].get<std::string>());
                }
            }

            std::string conId {gwLogin + ":" + gwPassword};
            {
                std::unique_lock lock(gwConnectionLock);

                if (auto&& conIt {gwConnections.find(conId)}; conIt != gwConnections.end()) {
                    con = conIt->second;
                    /*
                    con->Assign(gwLogin, gwPassword,
                        gwHosts, message.contains("port") ? message["port"].get<std::string>() : std::string{});
                        */
                    PSHT_INFO("Reuse connection: {} ( channel: {} ), {}", gwLogin, con->Channel(), *(int64_t*)this);

                } else {
                    con = gwConnections
                              .emplace(conId,
                                       std::make_shared<cgateway>(shared_from_this(),
                                                                  gwPoll,
                                                                  gwHook,
                                                                  gwLogin,
                                                                  gwPassword,
                                                                  gwHosts,
                                                                  message.contains("port")
                                                                      ? (message["port"].is_string()
                                                                             ? message["port"].get<std::string>()
                                                                             : std::to_string(message["port"].get<size_t>()))
                                                                      : std::string {}))
                              .first->second;

                    gwPoll->EnqueueGc(con);

                    PSHT_INFO("New connection: {} ( channel: {} ), {}", conId.c_str(), con->Channel().c_str(), *(int64_t*)this);
                }
            }

            if (con->Connect()) {
                auto&& src {message["message"].get<std::string>()};
                smpp::csms sms {0};
                sms.src(
                    message["source_ton"].get<uint8_t>(), message["source_npi"].get<uint8_t>(), message["source_addr"].get<std::string>());
                sms.dst(message["destination_ton"].get<uint8_t>(),
                        message["destination_npi"].get<uint8_t>(),
                        message["destination_addr"].get<std::string>());
                sms.encoding(smpp::param::cencoding::ucs2);
                sms.esm.Receipt(smpp::receipt_t::always);

                if (message.contains("receipt")) {
                    if (std::string && receipt {message["receipt"].get<std::string>()}; receipt == "none") {
                        sms.esm.Receipt(smpp::receipt_t::none);
                    } else if (receipt == "failure") {
                        sms.esm.Receipt(smpp::receipt_t::failure);
                    } else if (receipt == "success") {
                        sms.esm.Receipt(smpp::receipt_t::success);
                    }
                }

                //				auto MsgId{ con->MsgId() };
                auto&& chunks {sms.message.Split(sms.encoding.Encode(src))};

                sms.esm(chunks.size() == 1 ? 0 : 0x40);

                std::vector<uint32_t> MsgId;

                for (auto&& part : chunks) {
                    sms.command.sequence = con->Seq();
                    MsgId.emplace_back(sms.command.sequence);
                    sms.message(part);
                    auto&& tm_start {std::chrono::system_clock::now()};
                    if (auto res {con->Send(sms.Pack())}; res == 0) {
                        PSHT_INFO("Push SMS: {}, id: {}, seq: {}/{} ( src: {}, dst: {} ) ... success ({} msec)",
                                  sms.command.status,
                                  sms.command.id,
                                  sms.command.sequence,
                                  MsgId.size(),
                                  message["source_addr"].get<std::string>().c_str(),
                                  message["destination_addr"].get<std::string>().c_str(),
                                  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - tm_start)
                                      .count());
                        continue;
                    } else {
                        PSHT_ERROR("Push SMS: {}, id: {}, seq: {}/{} ( src: {}, dst: {} ) ... error ( {} )",
                                   sms.command.status,
                                   sms.command.id,
                                   sms.command.sequence,
                                   MsgId.size(),
                                   message["source_addr"].get<std::string>().c_str(),
                                   message["destination_addr"].get<std::string>().c_str(),
                                   std::strerror(-(int)res));
                        return {"400", json::object_t {{"id", MsgId}, {"error", strerror(-(int)res)}, {"channel", con->Channel()}}};
                    }
                }
                return {"201", json::object_t {{"id", MsgId}, {"status", "pending"}, {"channel", con->Channel()}}};
            } else {
                con->Close();
                PSHT_ERROR("Connection was lost ( {} )", con->Channel().c_str());
            }
            return {"400", json::object_t {{"error", "invalid gateway"}, {"channel", con->Channel()}}};
        } else {
            throw std::runtime_error("invalid `sender` field");
        }
    } catch (std::exception& ex) { return {"500", json::object_t {{"error", ex.what()}}}; }
    return {"400", json::object_t {{"error", "invalid message format"}}};
}

ssize_t csmppservice::cgateway::Send(const std::string& msg, std::string& response) {
    if (gwSocket) {
        ssize_t err {0};
        if (size_t nbytes {0}; (err = gwSocket.SocketSend(msg.data(), msg.length(), nbytes, 0)) == 0 and nbytes == msg.length()) {
            response.resize(512);
            if (err = gwSocket.SocketRecv(response.data(), sizeof(smpp::param::ccmd), nbytes, 0); err == 0 and nbytes == sizeof(smpp::param::ccmd)) {
                auto cmd {(smpp::param::ccmd*)response.data()};
                response.resize(cmd->Length());
                size_t nread {response.length() - sizeof(smpp::param::ccmd)};
                if (err = gwSocket.SocketRecv(response.data() + sizeof(smpp::param::ccmd), nread, nbytes, 0); err == 0 and nbytes == nread) {// NOLINT
                    return 0;
                }
            }
        }
        Close();
        return err;
    }
    return -EBADFD;
}

ssize_t csmppservice::cgateway::Send(const std::string& msg) {
    // std::unique_lock<decltype(gwSocketLock)> lock(gwSocketLock);
    if (gwSocket) {
        ssize_t err {0};
        if (size_t nbytes {0}; (err = gwSocket.SocketSend(msg.data(), msg.length(), nbytes, 0)) == 0) {
            return 0;
        }
        Close();
        return err;
    }
    return -EBADFD;
}

inline void csmppservice::cgateway::OnDeliveryStatus(const std::string& data) {
    std::string_view response {data};
    json::value_t status;
    if (auto&& [cmd] = smpp::cresponse<smpp::param::ccmd> {}(response); cmd.status == 0) {
        switch (cmd.id) {
            case smpp::cdelivery::id: {
                auto&& [report] = smpp::cresponse<smpp::cdelivery> {}(response);

                if (cmd.status == 0) {
                    status = json::object_t {
                        {"id", cmd.sequence}, {"status", "delivered"}, {"channel", Channel()}, {"data", report.message.Str()}};
                } else {
                    status = json::object_t {{"id", cmd.sequence}, {"status", "not_delivered"}, {"channel", Channel()}};
                }

                smpp::cdelivery_resp resp {cmd.sequence, {}};
                auto res = Send(resp.Pack());

                PSHT_INFO("Delivery SMS: status: {}, id: {}, seq: {} ( {} ), channel: {} ( {} )",
                          cmd.status,
                          cmd.id,
                          cmd.sequence,
                          report.message.Str().c_str(),
                          Channel().c_str(),
                          std::strerror(-(int)res));

                break;
            }
            case smpp::csms::resp_id: {
                auto&& [msg] = smpp::cresponse<smpp::param::cstring> {}(response);

                if (cmd.status == 0) {
                    status = json::object_t {{"id", cmd.sequence}, {"status", "accepted"}, {"channel", Channel()}, {"uid", msg.Str()}};
                    PSHT_INFO("Accept SMS: status: {}, id: {}, seq: {}, uid: {}, channel: {}",
                              cmd.status,
                              cmd.id,
                              cmd.sequence,
                              msg.Str().c_str(),
                              Channel().c_str());
                } else {
                    status = json::object_t {{"id", cmd.sequence}, {"status", "not_accepted"}, {"channel", Channel()}};
                    PSHT_INFO("Accept SMS: status: {}, id: {}, seq: {}, channel: {}",
                              cmd.status,
                              cmd.id,
                              cmd.sequence,
                              Channel().c_str());
                }
                break;
            }
            case smpp::cenquire::req_id: {
                smpp::cenquire resp {cmd.sequence};
                auto res = Send(resp.Pack());
                gwPPTimeout = std::time(nullptr) + gwTimeoutInterval;
                PSHT_DEBUG("Enquire Link ( ping ): status: {}, id: {}, seq: {}, channel: {}, timeout: {} sec ( {} )",
                           cmd.status,
                           cmd.id,
                           cmd.sequence,
                           Channel().c_str(),
                           gwPPTimeout,
                           std::strerror(-(int)res));
                return;
            }
            case smpp::cenquire::resp_id: {
                gwPPTimeout = std::time(nullptr) + gwTimeoutInterval;
                PSHT_DEBUG("Enquire Link ( pong ): status: {}, id: {}, seq: {}, channel: {}, timeout: {} sec ( {} )",
                           cmd.status,
                           cmd.id,
                           cmd.sequence,
                           Channel().c_str(),
                           gwPPTimeout,
                           std::strerror(0));
                return;
            }
            default: {
                PSHT_INFO("Status SMS: {}, id: {}, seq: {}", cmd.status, cmd.id, cmd.sequence);
                status = json::object_t {{"id", cmd.sequence}, {"status", "event"}, {"code", cmd.status}, {"channel", Channel()}};
                break;
            }
        };
    } else {
        PSHT_INFO("Invalid reply SMS: {}, id: {}, seq: {}", cmd.status, cmd.id, cmd.sequence);
        status = json::object_t {{"id", cmd.sequence}, {"status", "error"}, {"code", cmd.status}, {"channel", Channel()}};
    }

    if (gwHook) {
        gwHook->Push(status["status"].get<std::string>(), status["channel"].get<std::string>(), "POST", std::move(status), {{"Content-Type", "application/json"}, {"Connection", "close"}});
    }
}

void csmppservice::cgateway::OnGwReply([[maybe_unused]] fd_t fd, uint events) {
    if (gwSocket) {
        if (events == EPOLLIN) {
            ssize_t err {0};
            std::string response;
            response.resize(512);
            size_t nbytes {0};
            if (err = gwSocket.SocketRecv(response.data(), sizeof(smpp::param::ccmd), nbytes, 0); err == 0 and nbytes == sizeof(smpp::param::ccmd)) {
                auto cmd {(smpp::param::ccmd*)response.data()};
                response.resize(cmd->Length());
                size_t nread {response.length() - sizeof(smpp::param::ccmd)};
                if (err = gwSocket.SocketRecv(response.data() + sizeof(smpp::param::ccmd), nread, nbytes, MSG_WAITALL); err == 0 and nbytes == nread) {// NOLINT (need fix)
                    OnDeliveryStatus(response);
                    return;
                }
            }
            PSHT_ERROR("Reply error: {}", std::strerror(-(int)err));
        }
        Close();
    }
}

bool csmppservice::cgateway::IsLeaveUs(std::time_t now) {
    if (gwPPTimeout and gwPPTimeout <= now) {
        Close();
        PSHT_ERROR("Ping timeout {} <= {}, channel: {}, {}", gwPPTimeout, now, Channel(), *(int64_t*)this);
        return true;
    }

    if (gwPingTime and gwPingTime <= now) {
        if (gwSocket and gwSocket.GetErrorNo() == 0) {
            gwPingTime = std::time(nullptr) + gwPingInterval;
            smpp::cenquire enq {Seq(), smpp::cenquire::req_id};
            PSHT_DEBUG("Ping: {} ( {} sec )... {}", gwPingTime, gwPingInterval, Send(enq.Pack()) == 0 ? "success" : "fail");
        }
    }
    return false;
}

inline bool csmppservice::cgateway::Connect() {
    std::unique_lock<decltype(gwSocketLock)> lock(gwSocketLock);
    if (gwSocket and gwSocket.GetErrorNo() == 0) {
        return true;
    }
    gwPingTime = 0;
    gwSocket.SocketClose();
    for (auto&& sa : gwHosts) {
        ssize_t res {0};
        if (fd_t fd; (res = inet::TcpConnect(fd, sa, false, 10000)) == 0) {
            gwSocket = inet::csocket {fd, sa, inet::ssl_t {}, gwPoll};

            gwSocket.SetKeepAlive(true, 3, 10, 3);
            gwSocket.SetRecvTimeout(3000);
            gwSocket.SetSendTimeout(1000);

            /* Auth on the gateway */

            smpp::cbind ath {Seq()};
            ath.login(gwLogin);
            ath.password(gwPassword);

            std::string response;
            if (res = Send(ath.Pack(), response); res == 0) {
                std::string_view resp_data {response};
                auto&& [cmd, alpha] = smpp::cresponse<smpp::param::ccmd, smpp::param::cstring> {}(resp_data);
                if (cmd.status == 0) {
                    gwSocket.Poll()->PollAdd(gwSocket.Fd(), EPOLLIN, [this](auto&& PH1, auto&& PH2) {
                        OnGwReply(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
                    });
                    PSHT_INFO("Connect to {}  ... success", inet::GetAddress(sa));
                    /*
                     * Send Enquire link
                     */
                    gwPingTime = std::time(nullptr) + gwPingInterval;
                    gwPPTimeout = std::time(nullptr) + gwTimeoutInterval;
                    smpp::cenquire enq {Seq(), smpp::cenquire::req_id};

                    return Send(enq.Pack()) == 0;
                } else {
                    PSHT_INFO("Connect to {} Reply bind   ... error ( response status error )", inet::GetAddress(sa));
                }
            } else {
                PSHT_ERROR("Connect to {} Send bind  ... error ( {} )", inet::GetAddress(sa), std::strerror(-(int)res));
            }
            Close();
        } else {
            PSHT_ERROR("Connect to {}  ... error ( {} )", inet::GetAddress(sa), std::strerror(-(int)res));
        }
    }
    return {};
}

void csmppservice::cgateway::Assign(const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) {
    size_t nhash = std::_Hash_impl::hash(login) ^ std::_Hash_impl::hash(pwd) ^ std::_Hash_impl::hash(port);

    for (auto&& host : hosts) {
        nhash ^= std::_Hash_impl::hash(host);
    }

    if (nhash == gwHash) {
        return;
    }

    if (gwConId.empty()) {
        gwConId.clear();
        gwConId.resize(512);
        auto n = snprintf(gwConId.data(), 512, "%08X%08X%016lX", (uint32_t)(nhash % std::time(nullptr)), (uint32_t)std::time(nullptr), (uint64_t)nhash);
        gwConId.resize(n);
    }

    gwHash = nhash;
    gwLogin = login;
    gwPassword = pwd;

    gwHosts.clear();

    for (auto&& host : hosts) {
        sockaddr_storage sa;
        if (inet::GetSockAddr(sa, host, port, AF_INET) == 0) {
            gwHosts.emplace_back(sa);
        }
    }

    gwSocket.SocketClose();
}

csmppservice::cgateway::cgateway(const std::shared_ptr<csmppservice>& svc, const std::shared_ptr<inet::cpoll>& poll, const std::shared_ptr<cwebhook>& hook, const std::string& login, const std::string& pwd, const std::vector<std::string>& hosts, const std::string& port) :
    svc {svc},
    gwSocket {-1, {}},
    gwPoll {poll},
    gwHook {hook} {
    Assign(login, pwd, hosts, port);
}

csmppservice::cgateway::~cgateway() {}

csmppservice::csmppservice(const std::string& webhook) {
    if (!webhook.empty()) {
        gwHook = std::make_shared<cwebhook>(webhook);
    };
    gwPoll = std::make_shared<inet::cpoll>("smpp-worker");
    PSHT_INFO("Api SMPP ... enabled ( Delivery report is {} )", webhook.empty() ? "Disable" : "Enable");
}
