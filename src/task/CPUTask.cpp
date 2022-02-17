#include "CPUTask.h"
#include "CPUSheduler.h"
#include "../ServerGlobals.h"
#include "Poco/AtomicCounter.h"

namespace UniLib {
	namespace controller {


		CPUTask::CPUTask(CPUSheduler* cpuScheduler, size_t taskDependenceCount)
			: Task(taskDependenceCount), mScheduler(cpuScheduler)
		{
			assert(cpuScheduler);
			ServerGlobals::g_NumberExistingTasks++;
		}

		CPUTask::CPUTask(CPUSheduler* cpuScheduler)
			: Task(), mScheduler(cpuScheduler)
		{
			assert(cpuScheduler);
			ServerGlobals::g_NumberExistingTasks++;
		}

		CPUTask::CPUTask(size_t taskDependenceCount/* = 0*/)
			: Task(taskDependenceCount), mScheduler(ServerGlobals::g_CPUScheduler)
		{
			assert(mScheduler);
			ServerGlobals::g_NumberExistingTasks++;
		}

		CPUTask::~CPUTask()
		{
			ServerGlobals::g_NumberExistingTasks--;
		}

		void CPUTask::scheduleTask(TaskPtr own)
		{
			assert(mScheduler);
			if(!isTaskSheduled()) {
				mScheduler->sheduleTask(own);
				taskScheduled();
			}
		}
	}
}
