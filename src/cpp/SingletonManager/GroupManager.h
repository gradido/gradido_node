#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H

#include "../controller/GroupIndex.h"
#include "../controller/Group.h"

#include "Poco/Mutex.h"

#include <unordered_map>

class GroupManager
{
public:
	~GroupManager();

	static GroupManager* getInstance();

	int init(const char* groupIndexFileName);

	controller::Group* findGroup(const std::string& base58GroupHash);

protected:
	GroupManager();

	Poco::Mutex mWorkingMutex;
	bool mInitalized;
	controller::GroupIndex* mGroupIndex;
	std::unordered_map<std::string, controller::Group*> mGroups;

};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H