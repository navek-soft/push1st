// NOLINTBEGIN
#pragma once
#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace util::log {

template <class... Args>
void ____debug____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void ____trace____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void ____info____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void ____warning____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void ____error____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template <class... Args>
void ____critical____([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

}// namespace util::log
 // NOLINTEND
