#include <cinttypes>
#include <cstddef>

namespace util::log {

#pragma pack(push, 1)
struct interleaved_t {
    uint8_t magic;
    uint64_t s : 1;
    uint64_t e : 1;
    uint64_t r : 62;
    size_t l;
};

#pragma pack(pop)
}// namespace util::log
