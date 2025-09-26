#include "FuzzyTimer.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/types.h"
#include "../SystemExceptions.h"
#include "../iota/IotaExceptions.h"

#include "loguru/loguru.hpp"

FuzzyTimer::FuzzyTimer()
	: exit(false), mThread(new std::thread(&FuzzyTimer::run, this))
{
}

FuzzyTimer::~FuzzyTimer()
{
	mMutex.lock();
	exit = true;
	mMutex.unlock();
	if (mThread) {
		mThread->join();
		delete mThread;
		mThread = nullptr;
	}
	mRegisteredAtTimer.clear();
}

// -----------------------------------------------------------------------------------------------------

bool FuzzyTimer::addTimer(std::string name, TimerCallback* callbackObject, std::chrono::milliseconds timeInterval, int loopCount/* = -1*/)
{
	LOG_F(1, "%s", name.data());
	std::lock_guard _lock(mMutex);
	if (exit) return false;

	Timepoint now = std::chrono::system_clock::now();
	auto nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + timeInterval, TimerEntry(callbackObject, timeInterval, loopCount, name)));

	return true;
}

int FuzzyTimer::removeTimer(std::string name)
{
	if (mMutex.try_lock()) {
		mMutex.unlock();
	}
	else {
		throw CannotLockMutexAfterTimeout("FuzzyTimer::RemoveTimer", 100);
	}
	std::lock_guard _lock(mMutex);
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
	LOG_F(1, "removed %llu timer with name : %s", eraseCount, name.data());
	return eraseCount;
}

bool FuzzyTimer::move()
{
	std::lock_guard _lock(mMutex);
	if (exit) return false;
			
	auto it = mRegisteredAtTimer.begin();
	if (it == mRegisteredAtTimer.end()) return true;

	Timepoint now = std::chrono::system_clock::now();
	auto nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

	if (it->first <= nowMilliseconds) {
		if (!it->second.callback) {
			LOG_F(WARNING, "empty callback, name: %s", it->second.name.data());
		}
		else {
			TimerReturn ret = TimerReturn::NOT_SET;
			try {
				ret = it->second.callback->callFromTimer();
			}
			/*catch (MessageIdFormatException& ex) {
				LOG_F(ERROR, "message id exception: %s", ex.getFullString().data());
				ret = TimerReturn::EXCEPTION;
			}*/
			catch (GradidoBlockchainException& ex) {
				LOG_F(ERROR, "Gradido Blockchain Exception: %s", ex.getFullString().data());
				ret = TimerReturn::EXCEPTION;
			}
			catch (std::runtime_error& ex) {
				LOG_F(ERROR, "std::runtime_error: %s", ex.what());
				ret = TimerReturn::EXCEPTION;
			}
			
			if (it->second.nextLoop() && ret == TimerReturn::GO_ON) {
				mRegisteredAtTimer.insert(TIMER_TIMER_ENTRY(nowMilliseconds + it->second.timeInterval, it->second));
			}

			if (ret == TimerReturn::REPORT_ERROR) {
				LOG_F(
					ERROR, 
					"timer run report error: timer type: %s, timer name: %s",
					it->second.callback->getResourceType(),
					it->second.name.data()
				);
			}
			else if (ret == TimerReturn::EXCEPTION) {
				LOG_F(
					ERROR,
					"timer run throw a exception: timer type: %s, timer name: %s",
					it->second.callback->getResourceType(),
					it->second.name.data()
				);
			}
		}
		mRegisteredAtTimer.erase(it);					
	}

	return true;
}

void FuzzyTimer::stop()
{
	std::lock_guard _lock(mMutex);
	exit = true;
}
void FuzzyTimer::run()
{
	loguru::set_thread_name("FuzzyTimer");
	while (true) {
		if (!move()) {
			return;
		}
		std::this_thread::sleep_for(MAGIC_NUMBER_TIMER_THREAD_SLEEP_BETWEEN_MOVE_CALLS);
	}
}
