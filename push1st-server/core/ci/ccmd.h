#pragma once
#include <string>
#include <string_view>
#include <map>
#include <tuple>

namespace core {
	class ccmdline {
	private:
		std::map<std::string, std::tuple<std::string, char, int, const char*, const char*, const char*, std::string_view>> optionsList;
	public:
		ccmdline& option(const std::string& longname, char shortname = '\0', int require_arg_type = 0, const char* usage_help = nullptr, const char* usage_section = nullptr, const char* help_optionvalue = nullptr, const char* default_optionvalue = "");
		void operator()(int argc, char* argv[]);
		std::string_view operator[] (const std::string& longname) const;
		bool isset(const std::string& longname) const;
		const std::string_view program() const;
		const std::string_view pwd() const;
		size_t pid() const;
		void print(const std::string& banner = {}) const;
	};
}