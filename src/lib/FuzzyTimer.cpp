#include "FuzzyTimer.h"
#include "Poco/Timestamp.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

FuzzyTimer::FuzzyTimer()
	: exit(false)
{
	mThread.setName("FuzzyTimer");
	mThread.start(*this);
	//printf("FuzzyTimer::FuzzyTimer\n");
}

FuzzyTimer::~FuzzyTimer()
{
	//printf("FuzzyTimer::~FuzzyTimer\n");
	mMutex.lock();
	exit = true;
	mMutex.unlock();
	mThread.join();
	mRegisteredAtTimer.clear();
}

// -----------------------------------------------------------------------------------------------------

bool FuzzyTimer::addTimer(std::string name, TimerCallback* callbackObject, uint64_t timeIntervall, int loopCount/* = -1*/)
{
	//printf("FuzzyTimer::addTimer: %s\n", name.data());
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return false;

	Poco::Timestamp now;
	auto nowMilliseconds = (now.epochMicroseconds() / 1000);
	mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + timeIntervall, TimerEntry(callbackObject, timeIntervall, loopCount, name)));

	return true;
}

int FuzzyTimer::removeTimer(std::string name)
{
	//printf("[FuzzyTimer::removeTimer] with name: %s, exit: %d\n", name.data(), exit);
	if (mMutex.tryLock(100)) {
		mMutex.unlock();
	}
	else {
		return -2;
	}
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return -1;
	//printf("mRegisteredAtTimer size: %d\n", mRegisteredAtTimer.size());

	size_t eraseCount = 0;
				
	for (auto it = mRegisteredAtTimer.begin(); it != mRegisteredAtTimer.end();)
	{
		if (name == it->second.name)
		{
			it->second.callback = nullptr;
			it = mRegisteredAtTimer.erase(it);
			eraseCount++;
		}
		else {
			//printf("not removed: %s\n", it->second.name.data());
			it++;
		}
	}
	//printf("removed %d timer with name: %s\n", eraseCount, name.data());
	//printf("mRegisteredAtTimer size: %d\n", mRegisteredAtTimer.size());
	return eraseCount;
}

bool FuzzyTimer::move()
{
	//printf("FuzzyTimer::move\n");
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return false;
			
	auto it = mRegisteredAtTimer.begin();
	if (it == mRegisteredAtTimer.end()) return true;

	Poco::Timestamp now;
	auto nowMilliseconds = (now.epochMicroseconds() / 1000);

	if (it->first <= nowMilliseconds) {
		if (!it->second.callback) {
			printf("[FuzzyTimer::move] empty callback\n");
			printf("name: %s\n", it->second.name.data());
			//printf("type: %s\n", it->second.callback->getResourceType());
		}
		else {
			//printf("[FuzzyTimer::move] about to call: %s\n", it->second.name.data());
			Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
			TimerReturn ret;
			try {
				ret = it->second.callback->callFromTimer();
			}
			catch (GradidoBlockchainException& ex) {
				errorLog.error("[FuzzyTimer::move] Gradido Blockchain Exception: %s", ex.getFullString());
				ret = EXCEPTION;
			}
			catch (Poco::Exception& ex) {
				errorLog.error("[FuzzyTimer::move] Poco Exception: %s", ex.displayText());
				ret = EXCEPTION;
			} 
			catch (std::runtime_error& ex) {
				errorLog.error("[FuzzyTimer::move] std::runtime_error: %s", ex.what());
				ret = EXCEPTION;
			}
			if (it->second.nextLoop() && ret == GO_ON) {
				mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + it->second.timeIntervall, it->second));
			}

			if (ret == REPORT_ERROR) {
				errorLog.error(
					"[FuzzyTimer::move] timer run report error: timer type: %s, timer name: %s",
					std::string(it->second.callback->getResourceType()), it->second.name);				
			}
			else if (ret == EXCEPTION) {
				errorLog.error(
					"[FuzzyTimer::move] timer run throw a exception: timer type: %s, timer name: %s",
					std::string(it->second.callback->getResourceType()), it->second.name);
			}
		}
		mRegisteredAtTimer.erase(it);					
	}

	return true;
}

void FuzzyTimer::stop()
{
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	exit = true;
}
void FuzzyTimer::run()
{
	while (true) {
		if (!move()) {
			return;
		}
		Poco::Thread::sleep(MAGIC_NUMBER_TIMER_THREAD_SLEEP_BETWEEN_MOVE_CALLS_MILLISECONDS);
	}
}
