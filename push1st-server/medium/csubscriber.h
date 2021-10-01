#pragma once
#include "medium.h"
#include "cmessage.h"

class csubscriber
{
public:
	csubscriber(const std::string& ip, uint16_t port);
	virtual ~csubscriber();
	inline sid_t Id() { return subsId; }
	virtual void Push(const message_t& msg) = 0;
	virtual void GetUserInfo(std::string& userId, std::string& userData) = 0;
protected:
	std::string subsId;
};

