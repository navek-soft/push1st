#pragma once
#include "cchannel.h"

class cprivatechannel : public cchannel, public std::enable_shared_from_this<cprivatechannel>
{
public:
	cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
	virtual ~cprivatechannel();

	virtual void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
	virtual void UnSubscribe(const std::shared_ptr<csubscriber>& subscriber) override;
	virtual inline void GetUsers(usersids_t&, userslist_t&) override { ; }
private:
	std::unordered_map<std::string, std::weak_ptr<csubscriber>> chSubscribers;
};