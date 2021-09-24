#pragma once
#include "../core/cconfig.h"
#include <cstring>
#include <memory>

using data_t = const std::pair<std::shared_ptr<uint8_t[]>, size_t>&;
using sid_t = const std::string&;