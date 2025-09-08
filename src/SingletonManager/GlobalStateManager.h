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

	//! close leveldb file 
	void exit();

protected:
	GlobalStateManager();
	cache::StateThreadSafe mGlobalStateFile;
};


#endif // __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER
