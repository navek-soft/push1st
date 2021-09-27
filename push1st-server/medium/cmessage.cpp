#include "cmessage.h"
#include <atomic>

static std::atomic_uint64_t MsgId{ (uint64_t)std::chrono::system_clock::now().time_since_epoch().count() };

cmessage::cmessage(data_t&& msg, sid_t event, sid_t channel, sid_t producer, delivery_t delivery, std::time_t ttl) :
	Id{ ++MsgId }, 
	TimeArrival{ std::chrono::system_clock::now().time_since_epoch().count() }, 
	TimeExpireAt{ !ttl ? 0 : std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + ttl },
	Event{ event }, Channel{ channel }, Producer{ producer }, Delivery{ delivery }, Data{ std::move(msg) }
{
	;
}

cmessage::~cmessage() {
	;
}