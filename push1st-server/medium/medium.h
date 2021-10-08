#pragma once
#include "../core/cconfig.h"
#include <cstring>
#include <memory>

using array_t = std::pair<std::shared_ptr<uint8_t[]>, size_t>;
using data_t = const array_t&;
using sid_t = const std::string&;

static constexpr size_t MaxChannelNameLength{ 255 };
static constexpr size_t MaxHttpHeaderSize{ 8192 };

static inline std::string_view to_string(const data_t& data) {
	return std::string_view{ (const char*)data.first.get(), data.second };
}

static inline const char* str(hook_t::type tp) {
	//static const char* namesOfHook[]{ "register", "unregister", "join", "leave", "push","none"};
	switch (tp)
	{
	case hook_t::type::reg: return "register";
	case hook_t::type::unreg: return "unregister";
	case hook_t::type::join: return "join";
	case hook_t::type::leave: return "leave";
	case hook_t::type::push: return "push";
	default:
		break;
	}
	return "none";
}
