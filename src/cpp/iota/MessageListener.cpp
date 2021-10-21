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

        auto om = OrderingManager::getInstance();
        auto validator = om->getIotaMessageValidator();

        // set existing all on remove and change if exist, else they can be deleted
        // remove which set on remove from last run
        for(auto it = mStoredMessageIds.begin(); it != mStoredMessageIds.end(); it++) {
            while(MESSAGE_REMOVED == it->second) {
                it = mStoredMessageIds.erase(it);
                if(it == mStoredMessageIds.end()) {
                    break;
                }
            }
            it->second = MESSAGE_REMOVED;
        }

        // compare with existing
        for(auto it = messageIds.begin(); it != messageIds.end(); it++)
        {
            // check if it exist
            MessageId& messageId = *it;
            auto storedIt = mStoredMessageIds.find(messageId);
            if(storedIt != mStoredMessageIds.end()) {
                // update status if already exist
                storedIt->second = MESSAGE_EXIST;
            } else {
                if(mGroupAlias.size() > 0) {
                    messageId.groupAlias = mGroupAlias.data();
                }
                // add if not exist
                mStoredMessageIds.insert({messageId, MESSAGE_NEW});
                // and send to message validator
                validator->pushMessageId(messageId, mMessageType);
            }
        }
    }
}
