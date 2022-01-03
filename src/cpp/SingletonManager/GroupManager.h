#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_GROUP_MANAGER_H

#include "../controller/GroupIndex.h"
#include "../controller/Group.h"



#include "Poco/Mutex.h"
#include "Poco/AccessExpireCache.h"

#include <unordered_map>

/*!
 * 
 * @author Dario Rekowski
 * 
 * @date 2020-02-06
 *
 * @brief for getting controller::Group for group public key
 * 
 *  TODO: function to adding group if program is running (new group not in group.index file) or group removed from cache
 */

class GroupManager
{
public:
	~GroupManager();

	static GroupManager* getInstance();

	//! load group index file and fill hash list with group public key = folder name pairs
	int init(const char* groupIndexFileName);

	//! \brief getting group object from Poco::AccessExpireCache or create if not in cache and put in cache
	//!
	//! Poco::AccessExpireCache use his own mutex
	Poco::SharedPtr<controller::Group> findGroup(const std::string& groupAlias);

	std::vector<Poco::SharedPtr<controller::Group>> findAllGroupsWhichHaveTransactionsForPubkey(const std::string& pubkey);

	void exit();

protected:
	// load 
	GroupManager();

	bool mInitalized;
	controller::GroupIndex* mGroupIndex;
	
	std::unordered_map<std::string, Poco::SharedPtr<controller::Group>> mGroupMap;

	Poco::FastMutex mWorkMutex;
	

};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H