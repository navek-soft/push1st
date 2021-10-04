#pragma once
#include "cchannel.h"

class cpublicchannel : public cchannel, public std::enable_shared_from_this<cpublicchannel>
{
public:
	cpublicchannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
	virtual ~cpublicchannel();

	virtual void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
	virtual void UnSubscribe(const std::string& subscriber) override;
	virtual inline void GetUsers(usersids_t&, userslist_t&) override { ; }
};

