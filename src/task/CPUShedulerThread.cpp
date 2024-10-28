#include "CPUShedulerThread.h"
#include "CPUSheduler.h"
#include "Task.h"

#include "../ServerGlobals.h"

#ifdef _UNI_LIB_DEBUG
#include "gradido_blockchain/lib/Profiler.h"
#endif //_UNI_LIB_DEBUG
#include "gradido_blockchain/Application.h"

#include "loguru/loguru.hpp"

namespace task {
	CPUShedulerThread::CPUShedulerThread(CPUSheduler* parent, const char* name)
		: Thread(name), mParent(parent)
	{
		mName = name;
		mWaitingTask = mParent->getNextUndoneTask(this);
	}

	CPUShedulerThread::~CPUShedulerThread()
	{
	}

	int CPUShedulerThread::ThreadFunction()
	{
		loguru::set_thread_name(mName.data());
		
		while(mWaitingTask)
		{
				
#ifdef _UNI_LIB_DEBUG
			Profiler counter;
			//debug::CPUShedulerTasksLog* l = debug::CPUShedulerTasksLog::getInstance();
			std::string name = mWaitingTask->getName();
			//l->addTaskLogEntry((HASH)mWaitingTask.getResourcePtrHolder(), mWaitingTask->getResourceType(), mName.data(), name);
#endif
			try {
				int returnValue = mWaitingTask->run();
				if (!returnValue) {
					mWaitingTask->setTaskFinished();
				}
#ifdef _UNI_LIB_DEBUG
				if (mWaitingTask.use_count() != 1) {
					if (0 == strcmp(mWaitingTask->getResourceType(), "iota::ConfirmedMessageLoader")) {
						LOG_F(1, "iota::ConfirmedMessageLoader Task has another reference somewhere, name: %s", mWaitingTask->getName());
					}
					if (0 == strcmp(mWaitingTask->getResourceType(), "MessageToTransactionTask")) {
						LOG_F(1, "MessageToTransactionTask Task has another reference somewhere, name: %s", mWaitingTask->getName());
					}
				}
				//l->removeTaskLogEntry((HASH)mWaitingTask.getResourcePtrHolder());
			//	mSpeedLog.information("%s used on thread: %s by Task: %s of: %s (returned: %d)",
				//	counter.string(), mName, std::string(mWaitingTask->getResourceType()), name, returnValue);
#endif
			}
			catch (GradidoBlockchainException& ex) {
				LOG_F(FATAL, "gradido blockchain exception %s for task type: %s", ex.getFullString().data(), mWaitingTask->getResourceType());
#ifdef _UNI_LIB_DEBUG
				LOG_F(FATAL, "task name: %s", name.data());
#endif
				Application::terminate();
			}
			catch (std::exception& ex) {
				LOG_F(FATAL, "exception %s for task type: %s", ex.what(), mWaitingTask->getResourceType());
#ifdef _UNI_LIB_DEBUG
				LOG_F(FATAL, "task name: %s", name.data());
#endif
				Application::terminate();
			}
			ServerGlobals::g_NumberExistingTasks;
			mWaitingTask = mParent->getNextUndoneTask(this);
		}
		return 0;
	}

	void CPUShedulerThread::setNewTask(TaskPtr cpuTask)
	{
		{
			std::lock_guard _lock(mWorkMutex);
			mWaitingTask = cpuTask;
		}
		condSignal();
	}
}