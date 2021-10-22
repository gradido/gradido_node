#include "MessageListener.h"

#include "IotaWrapper.h"
#include "../SingletonManager/OrderingManager.h"
#include "../lib/Profiler.h"

namespace iota
{
    MessageListener::MessageListener(const std::string& index, MessageType messageType, long intervalMilliseconds/* = 2000*/)
    : mIndex(index), mMessageType(messageType),
      mListenerTimer(0, intervalMilliseconds), mErrorLog(Poco::Logger::get("errorLog"))
    {
        Poco::TimerCallback<MessageListener> callback(*this, &MessageListener::listener);
	    mListenerTimer.start(callback);

    }

    void MessageListener::listener(Poco::Timer& timer)
    {
        // main loop, called regulary in separate thread
        static const char* function_name = "MessageListener::listener";

        auto skipped = timer.skipped();
        if(skipped) {
            mErrorLog.error("[%s] %d calls skipped, function needs to much time %d", function_name, skipped, 0);
        }
        Profiler timeUsed;
        // collect message ids for index from iota
        auto messageIds = iota::getMessageIdsForIndexiation(mIndex);
        //printf("called getMessageIdsForIndexiation and get %d message ids %s\n", messageIds.size(), timeUsed.string().data());
		if (messageIds.size()) {
			updateStoredMessages(messageIds);
		}
    }

    void MessageListener::updateStoredMessages(const std::vector<MessageId>& currentMessageIds)
    {
		auto om = OrderingManager::getInstance();
		auto validator = om->getIotaMessageValidator();

		// set existing all on remove and change if exist, else they can be deleted
		// remove which set on remove from last run
		for (auto it = mStoredMessageIds.begin(); it != mStoredMessageIds.end(); it++) {
			while (MESSAGE_REMOVED == it->second) {
				it = mStoredMessageIds.erase(it);
				if (it == mStoredMessageIds.end()) {
					break;
				}
			}
			if (it == mStoredMessageIds.end()) {
				break;
			}
			it->second = MESSAGE_REMOVED;
		}

		// compare with existing
		for (auto it = currentMessageIds.begin(); it != currentMessageIds.end(); it++)
		{
			// check if it exist
			const MessageId& messageId = *it;
			auto storedIt = mStoredMessageIds.find(messageId);
			if (storedIt != mStoredMessageIds.end()) {
				// update status if already exist
				storedIt->second = MESSAGE_EXIST;
			}
			else {
				MessageId copy(messageId);
				if (mGroupAlias.size() > 0) {
					copy.groupAlias = mGroupAlias.data();
				}
				// add if not exist
				mStoredMessageIds.insert({ copy, MESSAGE_NEW });
				// and send to message validator
				validator->pushMessageId(copy, mMessageType);
			}
		}
    }
}
