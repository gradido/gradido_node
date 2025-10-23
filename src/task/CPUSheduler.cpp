#include "CPUSheduler.h"
#include "CPUShedulerThread.h"
#include "CPUTask.h"

#include "loguru/loguru.hpp"

#include <memory.h>

namespace task {

	CPUSheduler::CPUSheduler(uint8_t threadCount, const char* name)
		: mThreads(new CPUShedulerThread*[threadCount]), mThreadCount(threadCount), mName(name), mStopped(false)
	{
		char nameBuffer[10]; memset(nameBuffer, 0, 10);
		//uint8_t len = std:: min(strlen(name), 7);
		uint8_t len = strlen(name);
		if(len > 7) len = 7;
		memcpy(nameBuffer, name, len);
		int i = 0;
			
		for(int i = 0; i < threadCount; i++) {
			sprintf(&nameBuffer[len], "%.2d", i); 
			mThreads[i] = new CPUShedulerThread(this, nameBuffer);
			mThreads[i]->init();
		}
	}

	CPUSheduler::~CPUSheduler()
	{
		//printf("[CPUSheduler::~CPUSheduler]\n");
		for(int i = 0; i < mThreadCount; i++) {
			if (mThreads[i]) {
				mThreads[i]->exit();
				delete mThreads[i];
			}
		}
		delete[] mThreads;
		mThreadCount = 0;
		//printf("[CPUSheduler::~CPUSheduler] finished\n");
	}

	int CPUSheduler::sheduleTask(TaskPtr task)
	{
		{ // scoped lock
			std::lock_guard _lock(mCheckStopMutex);
			if (mStopped) {
				return 0;
			}
		} // scoped lock end
		CPUShedulerThread* t = NULL;
		// look at free worker threads
		if(task->isReady() && mFreeWorkerThreads.pop(t)) {
			// gave him the new task
			t->setNewTask(task);
		} else {
			// else put task to pending queue
			// printf("[CPUSheduler::sheduleTask] all %d threads in use \n", getThreadCount());
			if (mFreeWorkerThreads.empty()) {
				LOG_F(INFO, "sheduleTask in %s all %u threads in use, add to pending task list", mName.data(), getThreadCount());
			}
			{
				std::lock_guard _lock(mPendingTasksMutex);
				mPendingTasks.push_back(task);
			}
		}
		return 0;
	}

	void CPUSheduler::stop()
	{
		mCheckStopMutex.lock();
		mStopped = true;
		mCheckStopMutex.unlock();

		mPendingTasksMutex.lock();
		mPendingTasks.clear();
		mPendingTasksMutex.unlock();
	}
	TaskPtr CPUSheduler::getNextUndoneTask(CPUShedulerThread* Me)
	{
		// look at pending tasks
		TaskPtr task;
		mPendingTasksMutex.lock();
		for (std::list<TaskPtr>::iterator it = mPendingTasks.begin(); it != mPendingTasks.end(); it++) {
			if ((*it)->isReady()) {
				task = *it;
				mPendingTasks.erase(it);
				mPendingTasksMutex.unlock();
				return task;
			}
		}
		mPendingTasksMutex.unlock();
		// push thread to worker queue
		if (Me) {
			mFreeWorkerThreads.push(Me);
		}
			
		return nullptr;
	}
	void CPUSheduler::checkPendingTasks()
	{
		{ // scoped lock
			std::lock_guard _lock(mCheckStopMutex);
			if (mStopped) {
				return;
			}
		} // scoped lock end
		TaskPtr task = getNextUndoneTask(NULL);
		if (task) {
			sheduleTask(task);
		}
	}
}