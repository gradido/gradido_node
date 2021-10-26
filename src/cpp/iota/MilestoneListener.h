#ifndef GRADIDO_NODE_IOTA_MILESTONE_LISTENER
#define GRADIDO_NODE_IOTA_MILESTONE_LISTENER

#include "Poco/Timer.h"

namespace iota {

	//! MAGIC NUMBER: how many milestones should be loaded on startup from the past
#define MILESTONES_BOOTSTRAP_COUNT 5

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