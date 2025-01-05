#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER

#include "../cache/StateThreadSafe.h"

/*!
 * @author: Dario Rekowski
 * 
 * @date: 27.10.21
 * 
 * @brief: control global states which needed for the whole node server
 *         state a variables which must be saved persistent across node server runs, but change while running (difference to config)
 */

class GlobalStateManager
{
public:
	~GlobalStateManager();
	static GlobalStateManager* getInstance();

	// TODO: use a more generic approach instead writing update, get and member variable for every state 
	inline void updateLastIotaMilestone(int32_t lastIotaMilestone) { 
		mGlobalStateFile.updateState(cache::DefaultStateKeys::LAST_IOTA_MILESTONE, lastIotaMilestone);
	}
	inline int32_t getLastIotaMilestone() { return mGlobalStateFile.readInt32State(cache::DefaultStateKeys::LAST_IOTA_MILESTONE, 0); }

	//! close leveldb file 
	void exit();

protected:
	GlobalStateManager();
	cache::StateThreadSafe mGlobalStateFile;
};


#endif // __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER
