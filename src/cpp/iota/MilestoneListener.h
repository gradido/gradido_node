#ifndef GRADIDO_NODE_IOTA_MILESTONE_LISTENER
#define GRADIDO_NODE_IOTA_MILESTONE_LISTENER

#include "MessageListener.h"


namespace iota {
	class MilestoneListener : public MessageListener
	{
	public:
		MilestoneListener(long intervalMilliseconds = 500);
		~MilestoneListener();

		void listener(Poco::Timer& timer);

	protected:
		int32_t mLastKnownMilestoneIndex;
	};
}

#endif //GRADIDO_NODE_IOTA_MILESTONE_LISTENER