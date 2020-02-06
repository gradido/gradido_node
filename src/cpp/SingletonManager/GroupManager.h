#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H

#include "../controller/GroupIndex.h"
#include "../controller/Group.h"

class GroupManager
{
public:
	~GroupManager();

	static GroupManager* getInstance();

	int init(const char* groupIndexFileName);

protected:
	GroupManager();

	bool mInitalized;
	controller::GroupIndex* mGroupIndex;


};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H