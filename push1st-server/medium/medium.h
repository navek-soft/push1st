#pragma once
#include "../core/cconfig.h"
#include <cstring>
#include <memory>

using array_t = std::pair<std::shared_ptr<uint8_t[]>, size_t>;
using data_t = const array_t&;
using sid_t = const std::string&;

