#include "MilestoneListener.h"
#include "HTTPApi.h"
#include "MessageValidator.h"
#include <stdexcept>

namespace iota {
	MilestoneListener::MilestoneListener(long intervalMilliseconds /* = 500 */)
		: MessageListener("", MESSAGE_TYPE_MILESTONE, intervalMilliseconds), mLastKnownMilestoneIndex(0)
	{

	}

	MilestoneListener::~MilestoneListener()
	{

	}

	void MilestoneListener::listener(Poco::Timer& timer)
	{
		auto info = iotaHttp::getNodeInfo();
		bool firstRun = false;
		if (!mLastKnownMilestoneIndex) {
			mLastKnownMilestoneIndex = info.confirmedMilestoneIndex - MILESTONES_MAX_CACHED + 1;
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
		messageIds.reserve(messageCount);
		if (firstRun) {
			printf("bootstrap milestones: \n");
		}
		for (int i = mLastKnownMilestoneIndex; i <= info.confirmedMilestoneIndex; i++) {
			if (firstRun) {
				printf("\rrequest milestone %d (%d/%d)", i, i - mLastKnownMilestoneIndex+1, messageCount+1);
			}
			messageIds.push_back(iotaHttp::getMilestoneByIndex(i));
			
		}
		if (firstRun) {
			printf("\n");
		}
		mLastKnownMilestoneIndex = info.confirmedMilestoneIndex;	
		updateStoredMessages(messageIds);		
	}
}