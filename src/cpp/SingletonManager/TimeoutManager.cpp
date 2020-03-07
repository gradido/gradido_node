#include "TimeoutManager.h"
#include "../ServerGlobals.h"

TimeoutManager::TimeoutManager()
	: mTimer(0, ServerGlobals::g_TimeoutCheck)
{
	Poco::TimerCallback<TimeoutManager> callback(*this, &TimeoutManager::onTimer);
	mTimer.start(callback);
}

TimeoutManager::~TimeoutManager()
{

}

TimeoutManager* TimeoutManager::getInstance() 
{
	static TimeoutManager one;
	return &one;
}

void TimeoutManager::onTimer(Poco::Timer& timer)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	for (auto it = mTimeouts.begin(); it != mTimeouts.end(); it++) {
		if (!it->second.isNull()) {
			it->second->checkTimeout();
		}
	}
}

bool TimeoutManager::registerTimeout(Poco::SharedPtr<ITimeout> timeoutCheckReceiver)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	auto result = mTimeouts.insert(std::pair<ITimeout*, Poco::SharedPtr<ITimeout>>(timeoutCheckReceiver.get(), timeoutCheckReceiver));
	return result.second;
}

bool TimeoutManager::unregisterTimeout(Poco::SharedPtr<ITimeout> timeoutCheckReceiver)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	auto it = mTimeouts.find(timeoutCheckReceiver.get()); 
	if (it != mTimeouts.end()) {
		mTimeouts.erase(it);
		return true;
	}
	return false;
}