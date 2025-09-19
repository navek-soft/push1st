#pragma once
#include <map>
#include <string>
#include <string_view>
#include <tuple>

namespace core {
class ccmdline {
   private:
    std::map<std::string, std::tuple<std::string, char, int, const char*, const char*, const char*, std::string_view>> optionsList;

   public:
    ccmdline& Option(const std::string& longname, char shortname = '\0', int require_arg_type = 0, const char* usage_help = nullptr, const char* usage_section = nullptr, const char* help_optionvalue = nullptr, const char* default_optionvalue = "");
    void operator()(int argc, char* argv[]);
    std::string_view operator[](const std::string& longname) const;
    bool Isset(const std::string& longname) const;
    std::string_view Program() const;
    std::string_view Pwd() const;
    size_t PId() const;
    void Print(const std::string& banner = {}) const;
};
}// namespace core