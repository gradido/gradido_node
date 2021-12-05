#include "FuzzyTimer.h"
#include "Poco/Timestamp.h"
#include "../SingletonManager/LoggerManager.h"

namespace UniLib {
	namespace lib {

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

		bool FuzzyTimer::addTimer(std::string name, TimerCallback* callbackObject, uint64_t timeIntervall, int loopCount/* = -1*/)
		{
			Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
			if (exit) return false;

			Poco::Timestamp now;
			auto nowMilliseconds = (now.epochMicroseconds() / 1000);
			mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + timeIntervall, TimerEntry(callbackObject, timeIntervall, loopCount, name)));

			return true;
		}

		int FuzzyTimer::removeTimer(std::string name)
		{
			Poco::ScopedLock<Poco::Mutex> _lock(mMutex);
			if (exit) return -1;

			size_t eraseCount = 0;
			bool somethingErased = false;
			do {
				somethingErased = false;
				for (auto it = mRegisteredAtTimer.begin(); it != mRegisteredAtTimer.end(); it++)
				{
					if (name == it->second.name)
					{
						mRegisteredAtTimer.erase(it);
						eraseCount++;
						somethingErased = true;
						break;
					}
				}
			} while (somethingErased);

			return eraseCount;
		}

		bool FuzzyTimer::move()
		{
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
					printf("type: %s\n", it->second.callback->getResourceType());
				}
				else {
					TimerReturn ret = it->second.callback->callFromTimer();
					if (it->second.nextLoop() && ret == GO_ON) {
						mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + it->second.timeIntervall, it->second));
					}

					if (ret == REPORT_ERROR) {
						LoggerManager::getInstance()->mErrorLogging.error(
							"[Timer::move] timer run report error: timer type: %s, timer name: %s",
							std::string(it->second.callback->getResourceType()), it->second.name);
						//LOG_ERROR("report error from timer callback", DR_ERROR);
					}
				}
				mRegisteredAtTimer.erase(it);					
			}

			return true;
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
	}
}