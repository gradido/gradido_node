#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_TIMEOUT_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_TIMEOUT_MANAGER_H

#include "Poco/Timer.h"
#include "Poco/Mutex.h"

#include <map>

class ITimeout 
{
public:
	virtual void checkTimeout() = 0;
};

class TimeoutManager
{
public: 
	~TimeoutManager();

	static TimeoutManager* getInstance();

	void onTimer(Poco::Timer& timer);

	bool registerTimeout(ITimeout* timeoutCheckReceiver);
	bool unregisterTimeout(ITimeout* timeoutCheckReceiver);
	

protected:
	TimeoutManager();

	Poco::Timer mTimer;
	Poco::FastMutex mFastMutex;
	
	std::map<ITimeout*, ITimeout*> mTimeouts;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_TIMEOUT_MANAGER_H