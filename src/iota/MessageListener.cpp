#include "MessageListener.h"

#include "../SingletonManager/OrderingManager.h"
#include "../SingletonManager/CacheManager.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"
#include "../ServerGlobals.h"
#include "MqttClientWrapper.h"
#include "MessageParser.h"

using namespace rapidjson;

namespace iota
{
    MessageListener::MessageListener(const TopicIndex& index, std::chrono::milliseconds interval/* = 1000*/)
    : mIndex(index),
	  mInterval(interval),
	  mErrorLog(Poco::Logger::get("errorLog")),
	  mFirstRun(true)
    {
    }

	MessageListener::~MessageListener()
	{
		LOG_INFO("[iota::MessageListener] Stop Listen to: %s", mIndex.getHexString());
		lock();
#ifdef IOTA_WITHOUT_MQTT
		auto removedTimer = CacheManager::getInstance()->getFuzzyTimer()->removeTimer(mIndex.getBinString());
		if (removedTimer != 1 && removedTimer != -1) {
			LOG_ERROR("[iota::MessageListener] error removing timer, actually removed timer count: %d", removedTimer);
		}
#else
		MqttClientWrapper::getInstance()->unsubscribe(mIndex, this);
#endif
		unlock();
	}

	void MessageListener::run()
	{
		callFromTimer();
#ifdef IOTA_WITHOUT_MQTT
		CacheManager::getInstance()->getFuzzyTimer()->addTimer(mIndex.getBinString(), this, mInterval, -1);
#else
		MqttClientWrapper::getInstance()->subscribe(mIndex, this);
#endif 
		LOG_INFO("[iota::MessageListener] Listen to: %s", mIndex.getHexString());
	}

    /*void MessageListener::listener(Poco::Timer& timer)
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
	*/
	TimerReturn MessageListener::callFromTimer()
	{
		// main loop, called regularly in separate thread
		if (!tryLock()) return TimerReturn::GO_ON;
		//lock();
		static const char* function_name = "MessageListener::listener";

		// collect message ids for index from iota
		std::vector<MemoryBin> messageIds;
		try {
			messageIds = ServerGlobals::g_IotaRequestHandler->findByIndex(mIndex);
		}
		catch (...) {
			unlock();
			IotaRequest::defaultExceptionHandler(true);
			return TimerReturn::EXCEPTION;
		}
	
		//printf("called getMessageIdsForIndexiation and get %d message ids %s\n", messageIds.size(), timeUsed.string().data());
		if (messageIds.size()) {
			updateStoredMessages(messageIds);
		}
		mFirstRun = false;
		unlock();
		return TimerReturn::GO_ON;
	}

	void MessageListener::messageArrived(MQTTAsync_message* message, TopicType type)
	{
		if (TopicType::MESSAGES_INDEXATION == type) {
			MessageParser parser(message->payload, message->payloadlen);
			auto messageId = parser.getMessageId();
			if (addStoredMessage(messageId)) {
				MqttClientWrapper::getInstance()->subscribe(messageId, this);
			}
			// LOG_DEBUG("message arrived: %s", messageId.toHex());
		}
		else if (TopicType::MESSAGES_METADATA == type) {
			std::string messageString((const char*)message->payload, message->payloadlen);
			Document document;
			document.Parse((const char*)message->payload);

			rapidjson_helper::checkMember(document, "messageId", rapidjson_helper::MemberType::STRING, "iota milestone/metadata");
			
			if (!document.HasMember("referencedByMilestoneIndex")) {
				std::string messageIdString(document["messageId"].GetString());
				// LOG_DEBUG("metadata message received without milestone for message id: %s", messageIdString);
				return;
			}
			assert(document["referencedByMilestoneIndex"].IsInt());

			MessageId messageId(memory::Block::fromHex(document["messageId"].GetString()));			
			auto milestoneId = document["referencedByMilestoneIndex"].GetInt();
			LOG_DEBUG("[MessageListener::messageArrived] message arrived: %s confirmed in %d", messageId.toMemoryBin().convertToHex(), milestoneId);
			OrderingManager::getInstance()->getIotaMessageValidator()->messageConfirmed(messageId, milestoneId);
		}		
	}

	bool MessageListener::addStoredMessage(const MessageId &newMessageId)
	{
		auto om = OrderingManager::getInstance();
		auto validator = om->getIotaMessageValidator();		

		lock();
		assert(!mFirstRun);

		auto storedIt = mStoredMessageIds.find(newMessageId);
		if (storedIt != mStoredMessageIds.end())
		{
			// update status if already exist
			storedIt->second = MESSAGE_EXIST;
			LOG_ERROR("mqtt deliver a transaction the second time!", newMessageId.toMemoryBin().convertToHex());
			unlock();
			return false;
		}
		else
		{
			// add if not exist
			LOG_INFO("[MessageListener::updateStoredMessages] %s add message: %s", mIndex.getHexString(), newMessageId.toMemoryBin().convertToHex());
			mStoredMessageIds.insert({newMessageId, MESSAGE_NEW});
			// and send to message validator
			// validator->pushMessageId(newMessageId);
			// validator->signal();
		}
		unlock();
		return true;
	}

	void MessageListener::updateStoredMessages(std::vector<MemoryBin>& currentMessageIds)
  {
		auto om = OrderingManager::getInstance();
		auto validator = om->getIotaMessageValidator();
		if (mFirstRun) {
			validator->firstRunStart();
		}
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
			MessageId messageId(*it);
			auto storedIt = mStoredMessageIds.find(messageId);
			if (storedIt != mStoredMessageIds.end()) {
				// update status if already exist
				storedIt->second = MESSAGE_EXIST;
			}
			else {
				// add if not exist
				LOG_INFO(
					"[MessageListener::updateStoredMessages] %s add message: %s",
					mIndex.getHexString(),
					messageId.toMemoryBin().convertToHex()
				);
				mStoredMessageIds.insert({ messageId, MESSAGE_NEW });
				// and send to message validator
				validator->pushMessageId(messageId);
				atLeastOneNewMessagePushed = true;
			}
		}
		// signal condition separate to make sure that by loading from the past, all known messages are in list before processing start
		if (mFirstRun) {
			validator->firstRunFinished();
		}
		if (atLeastOneNewMessagePushed) {
			validator->signal();
		}
  }
}
