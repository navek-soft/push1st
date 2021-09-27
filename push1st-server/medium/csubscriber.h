#pragma once
#include "medium.h"

class cmessage;

class csubscriber
{
public:
	csubscriber(const std::string& ip, uint16_t port);
	virtual ~csubscriber();
	inline sid_t Id() { return subsId; }
	virtual void Push(const std::unique_ptr<cmessage>& msg) = 0;
protected:
	std::string subsId;
};

