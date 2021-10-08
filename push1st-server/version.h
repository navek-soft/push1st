#pragma once
#include <string>
#define SERVER_NAME_LONG "push1st"
#define SERVER_NAME_SHORT "push1st-srv"

namespace version {
	inline constexpr const char* application{ SERVER_NAME_LONG };
	inline constexpr const char* server{ SERVER_NAME_SHORT };
	extern const char* number;
	extern const char* build;
	extern const char* revision;
	extern const char* timestamp;

	inline std::string banner() { return std::string(server) + "-" + number + "-" + revision; }
};