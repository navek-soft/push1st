#include "ccmd.h"
#include <getopt.h>
#include <unistd.h>
#include <vector>

extern char* program_invocation_name;
extern char* program_invocation_short_name;

using namespace core;

ccmdline& ccmdline::option(const std::string& longname, char shortname, int require_arg_type, const char* usage_help, const char* usage_section, const char* help_optionvalue, const char* default_optionvalue) {
	optionsList.emplace(longname, std::make_tuple(longname, shortname, require_arg_type, usage_help, usage_section, help_optionvalue, default_optionvalue));
	return *this;
}

void ccmdline::operator()(int argc, char* argv[]) {
	std::vector<struct ::option> options_long(optionsList.size() + 1, { nullptr,0,nullptr,'\0' });
	std::string options_short;

	auto&& it{ options_long.begin() };

	for (auto&& o : optionsList) {
		it->has_arg = std::get<2>(o.second);
		it->name = std::get<0>(o.second).c_str();
		if (it->val = std::get<1>(o.second); it->val) {
			options_short.push_back(std::get<1>(o.second));
			switch (std::get<2>(o.second)) {
			case 2:
				options_short.push_back(':');
			case 1:
				options_short.push_back(':');
				break;
			}
		}
		++it;
	}


	int opt = -1, option_index = -1;
	while ((opt = getopt_long_only(argc, argv, options_short.c_str(), options_long.data(), &option_index)) != -1) {
		if (opt == '?' || opt == ':') { continue; }
		if (opt == 0) {
			if (options_long[option_index].has_arg == 1 && optarg == nullptr) {
				throw std::string(options_long[option_index].name);
			}
			std::get<6>(optionsList[options_long[option_index].name]) = optarg ? optarg : options_long[option_index].name;
		}
		else {
			size_t i{ 0 };
			for (auto&& a : options_long) {
				if (a.val == (int)opt) {
					if (a.has_arg == 1 && optarg == nullptr) {
						throw std::string(a.name);
					}
					std::get<6>(optionsList[options_long[i].name]) = optarg ? optarg : options_long[i].name;
					break;
				}
				++i;
			}
		}
	}
}

bool ccmdline::isset(const std::string& longname) const {
	if (auto&& it{ optionsList.find(longname) }; it != optionsList.end()) {
		return !std::get<6>(it->second).empty();
	}
	return false;
}

std::string_view ccmdline::operator[] (const std::string& longname) const {
	if (auto&& it{ optionsList.find(longname) }; it != optionsList.end()) {
		return std::get<6>(it->second);
	}
	return {};
}

const std::string_view ccmdline::program() const {
	return program_invocation_short_name;
}

const std::string_view ccmdline::pwd() const {
	return { program_invocation_name, static_cast<size_t>(program_invocation_short_name - program_invocation_name) };
}

size_t ccmdline::pid() const {
	return (size_t)getpid();
}

static inline std::string_view cmdline_trim_line(const std::string_view& v) {
	auto&& from{ v.begin() }, to{ v.end() };
	for (; std::isspace(*from) && from < to; from++) { ; }
	for (; to > from && std::isspace(*(to - 1)); to--) { ; }
	return { from, static_cast<size_t>(to - from) };
}

static inline std::vector<std::string> cmdline_split_help(const char* usage_help) {
	/* explode chains */
	std::vector<std::string> list;
	std::string_view string{ usage_help };
	std::size_t start = 0;
	std::size_t end = string.find('\n');
	if (usage_help) {
		while (end != std::string_view::npos)
		{
			list.emplace_back(cmdline_trim_line({ string.data() + start, static_cast<size_t>((string.data() + end) - (string.data() + start)) }));
			start = end + 1;
			end = string.find('\n', start);
		}
		list.emplace_back(cmdline_trim_line({ string.data() + start, string.length() - start }));
	}
	return list;
}

void ccmdline::print(const std::string& banner) const {
	std::multimap<std::string, std::tuple<std::string, char, std::vector<std::string>>> lines;
	std::string longname, section;
	char shortname;
	int require_arg_type;
	const char* usage_help, * usage_section, * arg_help;
	std::string_view val;
	std::vector<std::string> help_lines;
	size_t ln_max_size{ 0 }, line_num_spaces{ 0 };

	for (auto&& opt : optionsList) {
		std::tie(longname, shortname, require_arg_type, usage_help, usage_section, arg_help, val) = opt.second;
		if (arg_help) {
			longname.append("=").append(arg_help);
		}
		ln_max_size = std::max(ln_max_size, longname.length());
		lines.emplace(!usage_section ? "" : usage_section, std::make_tuple(longname, shortname, std::move(cmdline_split_help(usage_help))));
	}

	if (!banner.empty()) {
		printf("%s", banner.c_str());
		(banner.back() != '\n') && printf("\n");
	}
	else {
		printf("Usage: %s [OPTIONS] ... \n", program().data());
	}

	line_num_spaces = 7 + 2 + ln_max_size + 1;
	bool bOutput{ false };
	for (auto&& [s, h] : lines) {
		if (s.empty()) continue;

		bOutput |= true;

		if (section != s) {
			printf("\n  %s:\n\n", s.c_str());
			section = s;
		}
		std::tie(longname, shortname, help_lines) = h;

		if (shortname) { printf("   -%c, ", shortname); }
		else { printf("    %c  ", ' '); }

		if (!longname.empty()) { printf("--%-*s \t", ln_max_size, longname.c_str()); }
		else { printf("  %*s \t", ln_max_size, " "); }
		if (help_lines.size() == 1) {
			printf("%s", help_lines[0].c_str());
		}
		else if (help_lines.size() > 1) {
			printf("%s", help_lines[0].c_str());
			for (size_t i = 1; i < help_lines.size(); ++i) {
				printf("\n%*s\t%s", line_num_spaces, " ", help_lines[i].c_str());
			}
		}
		printf("\n");
	}
	if (bOutput) {
		/* multisections, separate unnamed section */
		printf("\n   \n\n");
	}
	
	for (auto&& [s, h] : lines) {
		if (!s.empty()) continue;
		std::tie(longname, shortname, help_lines) = h;

		if (shortname) { printf("   -%c, ", shortname); }
		else { printf("    %c  ", ' '); }

		if (!longname.empty()) { printf("--%-*s \t", ln_max_size, longname.c_str()); }
		else { printf("  %*s \t", ln_max_size, " "); }
		if (help_lines.size() == 1) {
			printf("%s", help_lines[0].c_str());
		}
		else if (help_lines.size() > 1) {
			printf("%s", help_lines[0].c_str());
			for (size_t i = 1; i < help_lines.size(); ++i) {
				printf("\n%*s\t%s", line_num_spaces, " ", help_lines[i].c_str());
			}
		}
		printf("\n");
	}
}