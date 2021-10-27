#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER

#include "../model/files/State.h"

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

	void updateLastIotaMilestone(int32_t lastIotaMilestone);
	inline int32_t getLastIotaMilestone() { return mLastIotaMilestone; }

protected:
	GlobalStateManager();
	model::files::State* mGlobalStateFile;
	bool mInitalized;

	int32_t mLastIotaMilestone;
};

#endif // __GRADIDO_NODE_SINGLETON_MANAGER_GLOBAL_STATE_MANAGER
