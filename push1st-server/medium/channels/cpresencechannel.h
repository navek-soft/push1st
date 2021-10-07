#pragma once
#include "cchannel.h"

class ccluster;

class cpresencechannel : public cchannel, public std::enable_shared_from_this<cpresencechannel>
{
public:
	cpresencechannel(const std::shared_ptr<cchannels>& channels, const std::shared_ptr<ccluster>& cluster, const std::string& cuid, const std::string& name, const app_t& app, autoclose_t mode);
	virtual ~cpresencechannel();
	virtual size_t Push(message_t&& msg);
	virtual void Subscribe(const std::shared_ptr<csubscriber>& subscriber) override;
	virtual void UnSubscribe(const std::string& subscriber) override;
	virtual void GetUsers(usersids_t&, userslist_t&) override;
	virtual void OnClusterJoin(const json::value_t& val) override;
	virtual void OnClusterLeave(const json::value_t& val) override;

protected:
	virtual void OnSubscriberLeave(const std::string& subscriber) override;
private:
	std::shared_ptr<ccluster> Cluster;
	std::shared_mutex UsersLock;
	std::unordered_map<std::string /* sess */, std::pair<std::string /* user */, std::string /* info */>> UsersList;
};