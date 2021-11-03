#ifndef GRADIDO_NODE_IOTA_MILESTONE_LISTENER
#define GRADIDO_NODE_IOTA_MILESTONE_LISTENER

#include "Poco/Timer.h"

namespace iota {

	//! MAGIC NUMBER: how many milestones should be loaded on startup from the past
#define MILESTONES_BOOTSTRAP_COUNT 20
	//! MAGIC NUMBER: how deep node server should go through parent and parent messages from milestone to find gradido transactions
	//! it maybe depends on how many messages iota is processing per second
	//! For every message to request from iota a new task will be started and run in g_IotaRequestCPUScheduler
#define MILESTONE_PARENT_MAX_RECURSION_DEEP 40

	class MilestoneListener
	{
	public:
		MilestoneListener(long intervalMilliseconds = 500);
		~MilestoneListener();

		void listener(Poco::Timer& timer);

	protected:
		Poco::Timer mListenerTimer;
		int32_t mLastKnownMilestoneIndex;
	};
}

#endif //GRADIDO_NODE_IOTA_MILESTONE_LISTENER
