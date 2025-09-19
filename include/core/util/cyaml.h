#pragma once
#include <yaml-cpp/yaml.h>

using yaml_t = YAML::Node;

namespace yaml {
static inline auto Load(const std::string& filename) {
    return YAML::LoadFile(filename);
}
static inline auto Store([[maybe_unused]] const std::string& filename, [[maybe_unused]] const yaml_t& root) {
    return false;
}
}// namespace yaml