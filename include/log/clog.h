#pragma once
// NOLINTBEGIN

#ifndef SPDLOG_DISABLE
#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>
#else
#endif

#include <core/config/cconfig.h>
#include <spdlog/fmt/fmt.h>

#define PSHT_DEFAULT_TRACE(fmt, ...)                                                \
    do {                                                                            \
        if (spdlog::get_level() <= spdlog::level::level_enum::trace) [[unlikely]] { \
            util::log::____trace____(fmt, ##__VA_ARGS__);                           \
        }                                                                           \
    } while (0);

#define PSHT_DEFAULT_DEBUG(fmt, ...)                                                \
    do {                                                                            \
        if (spdlog::get_level() <= spdlog::level::level_enum::debug) [[unlikely]] { \
            util::log::____debug____(fmt, ##__VA_ARGS__);                           \
        }                                                                           \
    } while (0);

#define PSHT_DEFAULT_INFO(fmt, ...)                                              \
    do {                                                                         \
        if (spdlog::get_level() <= spdlog::level::level_enum::info) [[likely]] { \
            util::log::____info____(fmt, ##__VA_ARGS__);                         \
        }                                                                        \
    } while (0);

#define PSHT_DEFAULT_WARNING(fmt, ...)                                           \
    do {                                                                         \
        if (spdlog::get_level() <= spdlog::level::level_enum::warn) [[likely]] { \
            util::log::____warning____(fmt, ##__VA_ARGS__);                      \
        }                                                                        \
    } while (0);

#define PSHT_DEFAULT_ERROR(fmt, ...)                                            \
    do {                                                                        \
        if (spdlog::get_level() <= spdlog::level::level_enum::err) [[likely]] { \
            util::log::____error____(fmt, ##__VA_ARGS__);                       \
        }                                                                       \
    } while (0);

#define PSHT_DEFAULT_CRITICAL(fmt, ...)                                              \
    do {                                                                             \
        if (spdlog::get_level() <= spdlog::level::level_enum::critical) [[likely]] { \
            util::log::____critical____(fmt, ##__VA_ARGS__);                         \
        }                                                                            \
    } while (0);

#define PSHT_TRACE(fmt, ...)                                                        \
    do {                                                                            \
        if (spdlog::get_level() <= spdlog::level::level_enum::trace) [[unlikely]] { \
            ____trace____(fmt, ##__VA_ARGS__);                                      \
        }                                                                           \
    } while (0);

#define PSHT_DEBUG(fmt, ...)                                                        \
    do {                                                                            \
        if (spdlog::get_level() <= spdlog::level::level_enum::debug) [[unlikely]] { \
            ____debug____(fmt, ##__VA_ARGS__);                                      \
        }                                                                           \
    } while (0);

#define PSHT_INFO(fmt, ...)                                                      \
    do {                                                                         \
        if (spdlog::get_level() <= spdlog::level::level_enum::info) [[likely]] { \
            ____info____(fmt, ##__VA_ARGS__);                                    \
        }                                                                        \
    } while (0);

#define PSHT_WARNING(fmt, ...)                                                   \
    do {                                                                         \
        if (spdlog::get_level() <= spdlog::level::level_enum::warn) [[likely]] { \
            ____warning____(fmt, ##__VA_ARGS__);                                 \
        }                                                                        \
    } while (0);

#define PSHT_ERROR(fmt, ...)                                                    \
    do {                                                                        \
        if (spdlog::get_level() <= spdlog::level::level_enum::err) [[likely]] { \
            ____error____(fmt, ##__VA_ARGS__);                                  \
        }                                                                       \
    } while (0);

#define PSHT_CRITICAL(fmt, ...)                                                      \
    do {                                                                             \
        if (spdlog::get_level() <= spdlog::level::level_enum::critical) [[likely]] { \
            ____critical____(fmt, ##__VA_ARGS__);                                    \
        }                                                                            \
    } while (0);

#define log_as(tag)                                                                                  \
   private:                                                                                          \
    static inline constexpr char __CLASS_TAG__[] = #tag;                                             \
    template <class... Args>                                                                         \
    static void ____debug____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {            \
        util::log::____debug____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...));    \
    }                                                                                                \
                                                                                                     \
    template <class... Args>                                                                         \
    static void ____trace____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {            \
        util::log::____trace____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...));    \
    }                                                                                                \
                                                                                                     \
    template <class... Args>                                                                         \
    static void ____info____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {             \
        util::log::____info____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...));     \
    }                                                                                                \
                                                                                                     \
    template <class... Args>                                                                         \
    static void ____warning____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {          \
        util::log::____warning____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...));  \
    }                                                                                                \
                                                                                                     \
    template <class... Args>                                                                         \
    static void ____error____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {            \
        util::log::____error____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...));    \
    }                                                                                                \
                                                                                                     \
    template <class... Args>                                                                         \
    static void ____critical____(fmt::format_string<Args...> fmt, Args&&... args) noexcept {         \
        util::log::____critical____("[{}] {}", #tag, fmt::format(fmt, std::forward<Args>(args)...)); \
    }

namespace util::log {

void initialize(const core::cconfig::clogger_t& config);

void uninitialize();

std::vector<std::string> buffer();
}// namespace util::log
#include <log/clog-inl.h>
// NOLINTEND
