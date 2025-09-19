#pragma once

#ifndef SPDLOG_HEADER_ONLY
#include <spdlog/sinks/cansicolor_mapsink.h>
#endif

#include <spdlog/details/os.h>
#include <spdlog/pattern_formatter.h>
namespace spdlog::sinks {

template <typename ConsoleMutex>
SPDLOG_INLINE cansicolor_mapsink<ConsoleMutex>::cansicolor_mapsink(std::array<FILE*, level::n_levels> target_files, color_mode mode) :
    targetFiles(target_files),
    sinkSync(ConsoleMutex::mutex()),
    formatter(details::make_unique<spdlog::pattern_formatter>())

{
    SetColorMode(mode);
    colors.at(level::trace) = ToString(white);
    colors.at(level::debug) = ToString(cyan);
    colors.at(level::info) = ToString(green);
    colors.at(level::warn) = ToString(yellow_bold);
    colors.at(level::err) = ToString(red_bold);
    colors.at(level::critical) = ToString(bold_on_red);
    colors.at(level::off) = ToString(reset);
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::SetColor(level::level_enum color_level, string_view_t color) {
    std::lock_guard<mutex_t> lock(sinkSync);
    colors.at(static_cast<size_t>(color_level)) = ToString(color);
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::log(const details::log_msg& msg) {
    std::lock_guard<mutex_t> lock(sinkSync);
    msg.color_range_start = 0;
    msg.color_range_end = 0;
    memory_buf_t formatted;
    auto target_file = targetFiles.at(static_cast<size_t>(msg.level));
    formatter->format(msg, formatted);
    if (shouldDoColors && msg.color_range_end > msg.color_range_start) {
        PrintRange(formatted, 0, msg.color_range_start, target_file);
        PrintCCode(colors.at(static_cast<size_t>(msg.level)), target_file);
        PrintRange(formatted, msg.color_range_start, msg.color_range_end, target_file);
        PrintCCode(reset, target_file);
        PrintRange(formatted, msg.color_range_end, formatted.size(), target_file);
    } else {
        PrintRange(formatted, 0, formatted.size(), target_file);
    }
    fflush(target_file);
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::flush() {
    std::lock_guard<mutex_t> lock(sinkSync);
    for (auto target_file : targetFiles) {
        fflush(target_file);
    }
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::set_pattern(const std::string& pattern) {
    std::lock_guard<mutex_t> lock(sinkSync);
    formatter = std::unique_ptr<spdlog::formatter>(new pattern_formatter(pattern));
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) {
    std::lock_guard<mutex_t> lock(sinkSync);
    formatter = std::move(sink_formatter);
}

template <typename ConsoleMutex>
SPDLOG_INLINE bool cansicolor_mapsink<ConsoleMutex>::ShouldColor() {
    return shouldDoColors;
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::SetColorMode(color_mode mode) {
    switch (mode) {
        case color_mode::always:
            shouldDoColors = true;
            return;
        case color_mode::automatic: {
            shouldDoColors = details::os::is_color_terminal();
            for (auto target_file : targetFiles) {
                shouldDoColors = shouldDoColors && details::os::in_terminal(target_file);
            }
            return;
        }
        case color_mode::never:
            shouldDoColors = false;
            return;
        default:
            shouldDoColors = false;
    }
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::PrintCCode(const string_view_t& color_code, FILE* target_file) {
    fwrite(color_code.data(), sizeof(char), color_code.size(), target_file);
}

template <typename ConsoleMutex>
SPDLOG_INLINE void cansicolor_mapsink<ConsoleMutex>::PrintRange(const memory_buf_t& formatted, size_t start, size_t end, FILE* target_file) {
    fwrite(formatted.data() + start, sizeof(char), end - start, target_file);
}

template <typename ConsoleMutex>
SPDLOG_INLINE std::string cansicolor_mapsink<ConsoleMutex>::ToString(const string_view_t& sv) {
    return {sv.data(), sv.size()};
}

// ansicolor_stdout_stderr_sink
template <typename ConsoleMutex>
SPDLOG_INLINE cansicolor_stdout_stderr_sink<ConsoleMutex>::cansicolor_stdout_stderr_sink(color_mode mode) :
    cansicolor_mapsink<ConsoleMutex>({stdout, stdout, stdout, stderr, stderr, stderr, stderr}, mode) {}

}// namespace spdlog::sinks
