#include "CPUSheduler.h"
#include "CPUShedulerThread.h"
#include "CPUTask.h"
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
		}
	}

	CPUSheduler::~CPUSheduler()
	{
		//printf("[CPUSheduler::~CPUSheduler]\n");
		for(int i = 0; i < mThreadCount; i++) {
			if (mThreads[i]) {
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
			Poco::ScopedLock<Poco::FastMutex> _lock(mCheckStopMutex);
			if (mStopped) {
				return 0;
			}
		} // scoped lock end
		CPUShedulerThread* t = NULL;
		// look at free worker threads
		if(task->isAllParentsReady() && mFreeWorkerThreads.pop(t)) {
			// gave him the new task
			t->setNewTask(task);
		} else {
			// else put task to pending queue
			printf("[CPUSheduler::sheduleTask] all %d threads in use \n", getThreadCount());
			mPendingTasksMutex.lock();
			mPendingTasks.push_back(task);
			mPendingTasksMutex.unlock();
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
			if ((*it)->isAllParentsReady()) {
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
			
		return TaskPtr();
	}
	void CPUSheduler::checkPendingTasks()
	{
		{ // scoped lock
			Poco::ScopedLock<Poco::FastMutex> _lock(mCheckStopMutex);
			if (mStopped) {
				return;
			}
		} // scoped lock end
		TaskPtr task = getNextUndoneTask(NULL);
		if (!task.isNull()) {
			sheduleTask(task);
		}
	}
}