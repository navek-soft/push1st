#include "cchannels.h"
#include "channels/cpublicchannel.h"
#include "channels/cprivatechannel.h"

std::shared_ptr<cchannel> cchannels::Register(channel_t type, const app_t& app, const std::string& name) {
	std::string chUid{ app->Id + "#" + name };
	std::unique_lock<decltype(Sync)> lock{ Sync };
	if (auto&& ch{ Channels.find(chUid) }; ch != Channels.end()) {
		return ch->second;
	}

	switch ((channel_t::type)type) {
	case channel_t::type::pub:
	{
		auto ch = std::dynamic_pointer_cast<cchannel>(std::make_shared<cpublicchannel>(shared_from_this(), chUid, name, app, autoclose_t::yes));
		Channels.emplace(chUid, ch);
		return ch;
	}
	case channel_t::type::prot:
	case channel_t::type::priv:
	{
		auto ch = std::dynamic_pointer_cast<cchannel>(std::make_shared<cprivatechannel>(shared_from_this(), chUid, name, app, autoclose_t::yes));
		Channels.emplace(chUid, ch);
		return ch;
	}
	case channel_t::type::pres:
		break;
	default:
		break;
	}
	return {};
}

void cchannels::UnRegister(const std::string& cuid) {
	std::unique_lock<decltype(Sync)> lock{ Sync };
	Channels.erase(cuid);
}

cchannels::cchannels() {

}

cchannels::~cchannels() {
	std::unique_lock<decltype(Sync)> lock{ Sync };
	Channels.clear();
}
