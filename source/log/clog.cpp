#include <inet/cinet.h>
#include <inet/csocket.h>
#include <log/clog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <version/version.h>

#include <regex>

#include "sink/cansicolor_mapsink.h"
#include "sink/cxorudp_sink.h"
#include "spdlog/pattern_formatter.h"

namespace util::log {

spdlog::level::level_enum SpdlogLevel(size_t level) {
    switch (level) {
        case 0:
            return spdlog::level::level_enum::off;
        case 1:
            return spdlog::level::level_enum::err;
        case 2:
            return spdlog::level::level_enum::warn;
        case 3:
            return spdlog::level::level_enum::info;
        case 4:
            return spdlog::level::level_enum::debug;
        case 5:
            return spdlog::level::level_enum::trace;
        default:
            break;
    }
    return spdlog::level::level_enum::trace;
}

class appid_formatter_flag : // NOLINT
    public spdlog::custom_flag_formatter
{
   public:
    appid_formatter_flag(const std::string& appId) {
        appid = appId;
    }

    void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
        std::string flagTxt {appid};
        dest.append(flagTxt.data(), flagTxt.data() + flagTxt.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<appid_formatter_flag>(appid);
    }

   private:
    std::string appid;
};

class tagid_formatter_flag :// NOLINT
    public spdlog::custom_flag_formatter
{
   public:
    tagid_formatter_flag(const std::string& tagId) {
        tagid = tagId;
    }

    void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
        std::string flagTxt {tagid};
        dest.append(flagTxt.data(), flagTxt.data() + flagTxt.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<tagid_formatter_flag>(tagid);
    }

   private:
    std::string tagid;
};

void EscapeJson(const spdlog::string_view_t& s, std::string& o) {
    o.clear();
    for (char c : s) {
        switch (c) {
            case '"':
                o += "\\\"";
                break;
            case '\\':
                o += "\\\\";
                break;
            case '\b':
                o += "\\b";
                break;
            case '\f':
                o += "\\f";
                break;
            case '\n':
                o += "\\n";
                break;
            case '\r':
                o += "\\r";
                break;
            case '\t':
                o += "\\t";
                break;
            default:
                o += c;
        }
    }
}

class escaped_formatter_flag : public spdlog::custom_flag_formatter {// NOLINT
   public:
    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
        EscapeJson(msg.payload, buffer);
        dest.append(buffer.data(), buffer.data() + buffer.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<escaped_formatter_flag>();
    }

   private:
    std::string buffer;
};

static inline bool StringReplace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string ReplacePatterns(const std::string& inputPattern) {
    std::string spdPattern {inputPattern};
    StringReplace(spdPattern, "%Timestamp", "%Y-%m-%d %H:%M:%S.%e");
    StringReplace(spdPattern, "%AppId", "%q");
    StringReplace(spdPattern, "%Tag", "%Q");
    StringReplace(spdPattern, "%Verbose", "%^%l%$");
    StringReplace(spdPattern, "%Message", "%v");
    StringReplace(spdPattern, "%EscMessage", "%V");
    return spdPattern;
}

template <typename T, typename S, typename... Args>
std::shared_ptr<T> MakeSink(const S& cfg, Args&&... args) {
    std::shared_ptr<T> sink = std::make_shared<T>(std::forward<Args>(args)...);

    if (cfg.pattern.empty()) {
        return sink;
    }

    auto spdPattern = ReplacePatterns(cfg.pattern);
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    if (not cfg.tag.empty()) {
        formatter->add_flag<tagid_formatter_flag>('Q', cfg.tag);
    }

    formatter->add_flag<escaped_formatter_flag>('V');
    formatter->add_flag<appid_formatter_flag>('q', Version.AppName).set_pattern(spdPattern);

    sink->set_level(SpdlogLevel(cfg.verbose));
    sink->set_formatter(std::move(formatter));

    return sink;
}

void initialize(const core::cconfig::clogger_t& config) {
    const int nthread = 1;
    spdlog::init_thread_pool(8192, nthread);

    std::vector<spdlog::sink_ptr> sinks {};

    if (auto&& sinkOut = config.stdoutSink; sinkOut) {
        sinks.emplace_back(MakeSink<spdlog::sinks::stdout_stderr_color_sink_st>(sinkOut.value()));
    }

    if (auto&& sinkFile = config.fileSink; sinkFile) {
        sinks.emplace_back(
            MakeSink<spdlog::sinks::rotating_file_sink_st>(sinkFile.value(), sinkFile->path, sinkFile->rotate, sinkFile->number));
    }

    if (auto&& sinkSys = config.syslogSink; sinkSys) {
        const static std::unordered_map<std::string, int> facilities {
            {"auth", LOG_AUTH},     {"authpriv", LOG_AUTHPRIV}, {"cron", LOG_CRON},     {"daemon", LOG_DAEMON},
            {"ftp", LOG_FTP},       {"kern", LOG_KERN},         {"lpr", LOG_LPR},       {"mail", LOG_MAIL},
            {"news", LOG_NEWS},     {"security", LOG_AUTH}, /* DEPRECATED */
            {"syslog", LOG_SYSLOG}, {"user", LOG_USER},         {"uucp", LOG_UUCP},     {"local0", LOG_LOCAL0},
            {"local1", LOG_LOCAL1}, {"local2", LOG_LOCAL2},     {"local3", LOG_LOCAL3}, {"local4", LOG_LOCAL4},
            {"local5", LOG_LOCAL5}, {"local6", LOG_LOCAL6},     {"local7", LOG_LOCAL7}};

        sinks.emplace_back(
            MakeSink<spdlog::sinks::syslog_sink_st>(sinkSys.value(), Version.AppName, LOG_PID, facilities.at(sinkSys->facility), true));
    }

    if (auto&& sinkRemote = config.remoteSink; sinkRemote) {
        if (std::smatch match; std::regex_match(sinkRemote->url, match, std::regex {R"(.*udp:\/\/(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+).*)"})) {
            sinks.emplace_back(
                MakeSink<spdlog::sinks::xorudp_sink_st>(sinkRemote.value(), match[1], (uint16_t)std::stoi(match[2].str())));
        } else if (std::smatch match; std::regex_match(sinkRemote->url, match, std::regex {R"(.*udp:\/\/(.+):([0-9]{4,5}).*)"})) {
            sockaddr_storage sa {};
            inet::ResolveAddress(match[1].str(), sa, match[2].str());
            if (auto ip = inet::GetIp(sa); not ip.empty()) {
                sinks.emplace_back(
                    MakeSink<spdlog::sinks::xorudp_sink_st>(sinkRemote.value(), ip, (uint16_t)std::stoi(match[2].str())));
            }
        }
    }

    auto logger = std::make_shared<spdlog::async_logger>(Version.AppName, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::level_enum::trace);// if level of sink is higher than this one, then it won't pass
    spdlog::set_default_logger(logger);

    spdlog::flush_every(std::chrono::seconds(5));
    spdlog::flush_on(spdlog::level::info);
    spdlog::flush_on(spdlog::level::err);
}

void uninitialize() {
    spdlog::shutdown();
}

std::vector<std::string> buffer() {
    std::vector<std::string> res;

    for (const auto& sink : spdlog::default_logger()->sinks()) {
        auto ringSink = std::dynamic_pointer_cast<spdlog::sinks::ringbuffer_sink_st>(sink);
        if (ringSink) {
            res = ringSink->last_formatted();
            break;
        }
    }

    return res;
}

}// namespace util::log
