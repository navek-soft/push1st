#pragma once
#include <filesystem>
#include <yaml-cpp/yaml.h>

using config_t = YAML::Node;

namespace config {
	static inline auto load(const std::filesystem::path& configfile) { return YAML::LoadFile(configfile); }
}