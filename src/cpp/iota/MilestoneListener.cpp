#include "MilestoneListener.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GlobalStateManager.h"
#include "HTTPApi.h"
#include <stdexcept>

namespace iota {
	MilestoneListener::MilestoneListener(long intervalMilliseconds /* = 500 */)
		: mListenerTimer(0, intervalMilliseconds), mLastKnownMilestoneIndex(0)
	{
		auto g_state = GlobalStateManager::getInstance();
		mLastKnownMilestoneIndex = g_state->getLastIotaMilestone();

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

		// if stored last know milestone is to far in the past, consider it a fresh start
		// TODO IMPORTANT!: first fetch missing transactions from other gradido nodes before continue with query iota for new transactions to prevent holes in the blockchain
		if (info.confirmedMilestoneIndex - mLastKnownMilestoneIndex > MILESTONES_BOOTSTRAP_COUNT) {
			mLastKnownMilestoneIndex = 0;
		}
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
		for (int i = mLastKnownMilestoneIndex + 1; i <= info.confirmedMilestoneIndex; i++) {
			if (firstRun) {
				printf("\rrequest milestone %d (%d/%d)", i, i - mLastKnownMilestoneIndex+1, messageCount+1);
			}
			else {
				std::clog << "request milestone: " << std::to_string(i) << std::endl;
			}

			auto milestoneId = iota::getMilestoneByIndex(i);
			Poco::SharedPtr<iota::Message> milestone = new iota::Message(milestoneId);
			if (milestone->requestFromIota()) {
				Poco::AutoPtr<iota::ConfirmedMessageLoader> task = new iota::ConfirmedMessageLoader(milestone, MILESTONE_PARENT_MAX_RECURSION_DEEP);
				task->scheduleTask(task);
			}
			else {
				LoggerManager::getInstance()->mErrorLogging.error("[MilestoneListener::listener] couldn't load milestone %d from iota", i);
			}
			
		}
		if (firstRun) {
			printf("\n");
		}
		mLastKnownMilestoneIndex = info.confirmedMilestoneIndex;
		auto g_state = GlobalStateManager::getInstance();
		g_state->updateLastIotaMilestone(mLastKnownMilestoneIndex);
	}
}