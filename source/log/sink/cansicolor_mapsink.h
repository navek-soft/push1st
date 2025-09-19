#pragma once

#include <spdlog/details/console_globals.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/sink.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
namespace spdlog::sinks {

template <typename ConsoleMutex>
class cansicolor_mapsink : public sink {
   public:
    using mutex_t = typename ConsoleMutex::mutex_t;
    cansicolor_mapsink(std::array<FILE*, level::n_levels>, color_mode mode);
    ~cansicolor_mapsink() override = default;

    cansicolor_mapsink(const cansicolor_mapsink& other) = delete;
    cansicolor_mapsink(cansicolor_mapsink&& other) = delete;

    cansicolor_mapsink& operator=(const cansicolor_mapsink& other) = delete;
    cansicolor_mapsink& operator=(cansicolor_mapsink&& other) = delete;

   public:
    void SetColor(level::level_enum color_level, string_view_t color);
    void SetColorMode(color_mode mode);
    bool ShouldColor();
    // NOLINTBEGIN
    void log(const details::log_msg& msg) override;
    void flush() override;
    void set_pattern(const std::string& pattern) final;
    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;
    // NOLINTEND

   public:
    const string_view_t reset = "\033[m";
    const string_view_t bold = "\033[1m";
    const string_view_t dark = "\033[2m";
    const string_view_t underline = "\033[4m";
    const string_view_t blink = "\033[5m";
    const string_view_t reverse = "\033[7m";
    const string_view_t concealed = "\033[8m";
    const string_view_t clear_line = "\033[K";

    const string_view_t black = "\033[30m";
    const string_view_t red = "\033[31m";
    const string_view_t green = "\033[32m";
    const string_view_t yellow = "\033[33m";
    const string_view_t blue = "\033[34m";
    const string_view_t magenta = "\033[35m";
    const string_view_t cyan = "\033[36m";
    const string_view_t white = "\033[37m";

    const string_view_t on_black = "\033[40m";
    const string_view_t on_red = "\033[41m";
    const string_view_t on_green = "\033[42m";
    const string_view_t on_yellow = "\033[43m";
    const string_view_t on_blue = "\033[44m";
    const string_view_t on_magenta = "\033[45m";
    const string_view_t on_cyan = "\033[46m";
    const string_view_t on_white = "\033[47m";

    const string_view_t yellow_bold = "\033[33m\033[1m";
    const string_view_t red_bold = "\033[31m\033[1m";
    const string_view_t bold_on_red = "\033[1m\033[41m";

   private:
    std::array<FILE*, level::n_levels> targetFiles;
    mutex_t& sinkSync;
    bool shouldDoColors;
    std::unique_ptr<spdlog::formatter> formatter;
    std::array<std::string, level::n_levels> colors;

   private:
    void PrintCCode(const string_view_t& color_code, FILE* target_file);
    void PrintRange(const memory_buf_t& formatted, size_t start, size_t end, FILE* target_file);
    static std::string ToString(const string_view_t& sv);
};

template <typename ConsoleMutex>
class cansicolor_stdout_stderr_sink : public cansicolor_mapsink<ConsoleMutex> {
   public:
    explicit cansicolor_stdout_stderr_sink(color_mode mode = color_mode::automatic);
};

using ansicolor_stdout_stderr_sink_mt = cansicolor_stdout_stderr_sink<details::console_mutex>;
using ansicolor_stdout_stderr_sink_st = cansicolor_stdout_stderr_sink<details::console_nullmutex>;

using stdout_stderr_color_sink_mt = ansicolor_stdout_stderr_sink_mt;
using stdout_stderr_color_sink_st = ansicolor_stdout_stderr_sink_st;

// NOLINTBEGIN >>> to save spdlog`s function code style
template <typename Factory>
SPDLOG_INLINE std::shared_ptr<logger> stdout_stderr_color_mt(const std::string& logger_name, color_mode mode) {
    return Factory::template create<sinks::stdout_stderr_color_sink_mt>(logger_name, mode);
}

template <typename Factory>
SPDLOG_INLINE std::shared_ptr<logger> stdout_stderr_color_st(const std::string& logger_name, color_mode mode) {
    return Factory::template create<sinks::stdout_stderr_color_sink_st>(logger_name, mode);
}
// NOLINTEND

}// namespace spdlog::sinks

#ifdef SPDLOG_HEADER_ONLY
#include "cansicolor_mapsink-inl.h"
#endif
