#pragma once
#include <stdint.h>

#include <string>
#include <vector>
namespace ci {
class cbase62 {
   public:
    static std::string Encode(const uint8_t* source, std::size_t length);
    static std::vector<uint8_t> Decode(const uint8_t* source, std::size_t length);
};
}// namespace ci