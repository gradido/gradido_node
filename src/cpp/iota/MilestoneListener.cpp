#include "MilestoneListener.h"
#include "HTTPApi.h"
#include <stdexcept>

namespace iota {
	MilestoneListener::MilestoneListener(long intervalMilliseconds /* = 500 */)
		: mListenerTimer(0, intervalMilliseconds), mLastKnownMilestoneIndex(0)
	{
		Poco::TimerCallback<MilestoneListener> callback(*this, &MilestoneListener::listener);
		mListenerTimer.start(callback);

	}

	MilestoneListener::~MilestoneListener()
	{

	}

	void MilestoneListener::listener(Poco::Timer& timer)
	{
		auto info = iota::getNodeInfo();
		bool firstRun = false;
		if (!mLastKnownMilestoneIndex) {
			mLastKnownMilestoneIndex = info.confirmedMilestoneIndex - MILESTONES_BOOTSTRAP_COUNT + 1;
			firstRun = true;
		}
		// no new milestone by iota so we can early exit here
		if (mLastKnownMilestoneIndex == info.confirmedMilestoneIndex) {
			return;
		}
		std::vector<MessageId> messageIds;
		size_t messageCount = (size_t)(info.confirmedMilestoneIndex - mLastKnownMilestoneIndex);
		if ((int32_t)messageCount != info.confirmedMilestoneIndex - mLastKnownMilestoneIndex) {
			throw std::runtime_error("invalid message count");
		}
		if (firstRun) {
			printf("bootstrap milestones: \n");
		}
		for (int i = mLastKnownMilestoneIndex; i <= info.confirmedMilestoneIndex; i++) {
			if (firstRun) {
				printf("\rrequest milestone %d (%d/%d)", i, i - mLastKnownMilestoneIndex+1, messageCount+1);
			}
			else {
				std::clog << "request milestone: " << std::to_string(i) << std::endl;
			}

			Poco::AutoPtr<iota::ConfirmedMessageLoader> task = new iota::ConfirmedMessageLoader(iota::getMilestoneByIndex(i), 2);
			task->scheduleTask(task);
			
		}
		if (firstRun) {
			printf("\n");
		}
		mLastKnownMilestoneIndex = info.confirmedMilestoneIndex;	
	}
}