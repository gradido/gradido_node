#include "FuzzyTimer.h"
#include "Poco/Timestamp.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/types.h"
#include "../SystemExceptions.h"
#include "../iota/IotaExceptions.h"

FuzzyTimer::FuzzyTimer()
	: exit(false)
{
	mThread.setName("FuzzyTimer");
	mThread.start(*this);
}

FuzzyTimer::~FuzzyTimer()
{
	mMutex.lock();
	exit = true;
	mMutex.unlock();
	mThread.join();
	mRegisteredAtTimer.clear();
}

// -----------------------------------------------------------------------------------------------------

bool FuzzyTimer::addTimer(std::string name, TimerCallback* callbackObject, std::chrono::milliseconds timeInterval, int loopCount/* = -1*/)
{
	LOG_DEBUG("[FuzzyTimer::addTimer] %s", name);
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return false;

	Timepoint now;
	auto nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + timeInterval, TimerEntry(callbackObject, timeInterval, loopCount, name)));

	return true;
}

int FuzzyTimer::removeTimer(std::string name)
{
	if (mMutex.tryLock(100)) {
		mMutex.unlock();
	}
	else {
		throw CannotLockMutexAfterTimeout("FuzzyTimer::RemoveTimer", 100);
	}
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return -1;

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
			it++;
		}
	}
	LOG_DEBUG("[FuzzyTimer::removeTimer] removed %d timer with name: %s", eraseCount, name);
	return eraseCount;
}

bool FuzzyTimer::move()
{
	Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
	if (exit) return false;
			
	auto it = mRegisteredAtTimer.begin();
	if (it == mRegisteredAtTimer.end()) return true;

	Timepoint now;
	auto nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

	if (it->first <= nowMilliseconds) {
		if (!it->second.callback) {
			LOG_WARN("[FuzzyTimer::move] empty callback, name: %s", it->second.name);
		}
		else {
			Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
			TimerReturn ret = TimerReturn::NOT_SET;
			try {
				ret = it->second.callback->callFromTimer();
			}
			catch (GradidoBlockchainException& ex) {
				errorLog.error("[FuzzyTimer::move] Gradido Blockchain Exception: %s", ex.getFullString());
				ret = TimerReturn::EXCEPTION;
			}
			catch (Poco::Exception& ex) {
				errorLog.error("[FuzzyTimer::move] Poco Exception: %s", ex.displayText());
				ret = TimerReturn::EXCEPTION;
			} 
			catch (std::runtime_error& ex) {
				errorLog.error("[FuzzyTimer::move] std::runtime_error: %s", ex.what());
				ret = TimerReturn::EXCEPTION;
			}
			catch (MessageIdFormatException& ex) {
				errorLog.error("[FuzzyTimer::move] ... exception");
				ret = TimerReturn::EXCEPTION;
			}
			if (it->second.nextLoop() && ret == TimerReturn::GO_ON) {
				mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + it->second.timeInterval, it->second));
			}

			if (ret == TimerReturn::REPORT_ERROR) {
				errorLog.error(
					"[FuzzyTimer::move] timer run report error: timer type: %s, timer name: %s",
					std::string(it->second.callback->getResourceType()), it->second.name);				
			}
			else if (ret == TimerReturn::EXCEPTION) {
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
