#include "MessageListener.h"

#include "../SingletonManager/OrderingManager.h"
#include "../SingletonManager/CacheManager.h"
#include "../lib/Profiler.h"
#include "HTTPApi.h"

namespace iota
{
    MessageListener::MessageListener(const std::string& index, long intervalMilliseconds/* = 2000*/)
    : mIndex(index),
      //mListenerTimer(0, intervalMilliseconds), 
		mErrorLog(Poco::Logger::get("errorLog"))
    {
        //Poco::TimerCallback<MessageListener> callback(*this, &MessageListener::listener);
	    //mListenerTimer.start(callback);
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(mIndex, this, intervalMilliseconds, -1);
    }

	MessageListener::~MessageListener()
	{
		//printf("[iota::~MessageListener]\n");
		lock();
		if (CacheManager::getInstance()->getFuzzyTimer()->removeTimer(mIndex) != 1) {
			printf("[iota::~MessageListener] error removing timer\n");
		}
		//mListenerTimer.stop();
		unlock();
	}

    void MessageListener::listener(Poco::Timer& timer)
    {
        // main loop, called regulary in separate thread
		lock();
        static const char* function_name = "MessageListener::listener";

        int skipped = timer.skipped();
        if(skipped) {
            mErrorLog.error("[%s] %d calls skipped, function needs to much time", std::string(function_name), skipped);
        }
        Profiler timeUsed;
        // collect message ids for index from iota
        auto messageIds = findByIndex(mIndex);
        //printf("called getMessageIdsForIndexiation and get %d message ids %s\n", messageIds.size(), timeUsed.string().data());
		if (messageIds.size()) {
			updateStoredMessages(messageIds);
		}
		unlock();
    }

	UniLib::lib::TimerReturn MessageListener::callFromTimer()
	{
		// main loop, called regulary in separate thread
		if (!tryLock()) return UniLib::lib::GO_ON;
		//lock();
		static const char* function_name = "MessageListener::listener";

		Profiler timeUsed;
		// collect message ids for index from iota
		auto messageIds = findByIndex(mIndex);
		//printf("called getMessageIdsForIndexiation and get %d message ids %s\n", messageIds.size(), timeUsed.string().data());
		if (messageIds.size()) {
			updateStoredMessages(messageIds);
		}
		unlock();
		return UniLib::lib::GO_ON;
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
		bool atLeastOneNewMessagePushed = false;
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
				// add if not exist
				printf("[MessageListener::updateStoredMessages] add message: %s\n", messageId.toHex().data());
				mStoredMessageIds.insert({ messageId, MESSAGE_NEW });
				// and send to message validator
				validator->pushMessageId(messageId);
				atLeastOneNewMessagePushed = true;
			}
		}
		// signal condition separate to make sure that by loading from the past, all known messages are in list before processing start
		if (atLeastOneNewMessagePushed) {
			validator->signal();
		}
    }
}
