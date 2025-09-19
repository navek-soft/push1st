#pragma once
#include <core/util/crandom.h>
#include <log/cinterleaved.h>
#include <spdlog/common.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/udp_sink.h>

#include <iostream>
#include <queue>

namespace spdlog ::sinks {

template <typename Mutex>
class cxorudp_sink : public udp_sink<Mutex> {
    using interleaved_t = util::log::interleaved_t;

   public:
    cxorudp_sink(const std::string& host, uint16_t port) : udp_sink<Mutex>(udp_sink_config(host, port)) {
        std::cout << "host: " << host << " port: " << port << std::endl;
    }

    ~cxorudp_sink() override = default;

   protected:
    inline std::queue<std::string_view> Pack(std::string_view str, size_t mtu) const noexcept {
        std::queue<std::string_view> res;
        while (not str.empty()) {
            auto len = std::min(mtu, str.size());
            res.push(str.substr(0, len));
            str.remove_prefix(len);
        }
        return res;
    }

    inline void SendInterleaved(std::string_view fragment, bool start, bool end, uint32_t random, size_t len) {
        interleaved_t interleaved {.magic = magic, .s = start, .e = end, .r = random, .l = len};
        udp_sink<Mutex>::client_.send((const char*)(&interleaved), sizeof(interleaved), MSG_MORE);
        udp_sink<Mutex>::client_.send(fragment.data(), fragment.size());
    }

    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        Xor(formatted.data(), formatted.size());
        bool start = true;
        auto random = core::Rand<uint64_t>();
        for (auto fragments {Pack({formatted.data(), formatted.size()}, 512)}; not fragments.empty(); fragments.pop()) {
            SendInterleaved(fragments.front(), start, fragments.size() == 1, random, formatted.size());
            start = false;
        }
    }

    void Xor(void* buffer, size_t size) const noexcept {
        auto data = static_cast<uint8_t*>(buffer);
        auto seed = '@';
        for (size_t i = 0; i < size; ++i) {
            data[i] ^= seed;
        }
    }

   private:
    static inline const uint8_t magic {'$'};
};

using xorudp_sink_mt = cxorudp_sink<std::mutex>;
using xorudp_sink_st = cxorudp_sink<spdlog::details::null_mutex>;

}// namespace spdlog::sinks
