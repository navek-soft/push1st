#pragma once
#include "medium.h"
#include "cmessage.h"

class csubscriber
{
public:
	csubscriber(const std::string& ip, uint16_t port, const std::string& prefix = {});
	virtual ~csubscriber();
	inline sid_t Id() { return subsId; }
	virtual inline fd_t GetFd() = 0;
	virtual inline bool IsConnected(std::time_t) = 0;
	virtual ssize_t Push(const message_t& msg) = 0;
	virtual void GetUserInfo(std::string& userId, std::string& userData) = 0;
	virtual void Disconnect() = 0;
protected:
	std::string subsId;
};

