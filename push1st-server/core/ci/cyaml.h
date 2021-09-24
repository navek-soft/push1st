#pragma once
#include <yaml-cpp/yaml.h>

using yaml_t = YAML::Node;

namespace yaml {
	static inline auto load(const std::string& filename) { return YAML::LoadFile(filename); }
	static inline auto store(const std::string& filename, const yaml_t& root) { return false; }
}