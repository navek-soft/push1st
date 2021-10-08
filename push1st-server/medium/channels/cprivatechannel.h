#pragma once
#include "cchannel.h"

class ccluster;

class cprivatechannel : public cchannel, public std::enable_shared_from_this<cprivatechannel>
{
public:
	cprivatechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
	virtual ~cprivatechannel();
	virtual size_t Push(message_t&& msg);
	virtual void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
	virtual void UnSubscribe(const std::string& subscriber) override;
	virtual inline void GetUsers(usersids_t&, userslist_t&) override { ; }
protected:
	virtual void OnSubscriberLeave(const std::string& subscriber) override;
private:
	std::shared_ptr<ccluster> Cluster;
};