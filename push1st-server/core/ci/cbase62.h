#pragma once
#include <string>
#include <vector>

namespace ci {
	class cbase62 {
	public:
		static std::string encode(const uint8_t* source, std::size_t length);
		static std::vector<uint8_t> decode(const uint8_t* source, std::size_t length);
	};
}