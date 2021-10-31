#include "Message.h"

#include "../lib/DataTypeConverter.h"
#include "../lib/BinTextConverter.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "../SingletonManager/OrderingManager.h"
#include "../ServerGlobals.h"
#include "ConfirmedMessagesCache.h"
#include "HTTPApi.h"

using namespace rapidjson;

namespace iota {
	// ****************** Message Id ******************************
	void MessageId::fromByteArray(const char* byteArray)
	{
		assert(byteArray);
		memcpy(messageId, byteArray, 32);
	}

	void MessageId::fromHex(std::string hex)
	{
		auto binString = convertHexToBin(hex);
		fromByteArray(binString.data());
	}

	MemoryBin* MessageId::toMemoryBin()
	{
		auto result = MemoryManager::getInstance()->getFreeMemory(32);
		memcpy(result->data(), messageId, 32);
		return result;
	}

	std::string MessageId::toHex() const
	{
		return DataTypeConverter::binToHex((unsigned char*)messageId, 32).substr(0, 64);
	}

	bool MessageId::isEmpty() const
	{
		return 
			messageId[0] == 0 &&
			messageId[1] == 0 &&
			messageId[2] == 0 &&
			messageId[3] == 0;
	}

	// ******************* Message ***************************************
	Message::Message(MessageId id)
		: mId(id), mData(nullptr), mMilestoneId(0), mMilestoneTimestamp(0), mFetched(false)
	{

	}

	Message::~Message()
	{
		if (mData) {
			MemoryManager::getInstance()->releaseMemory(mData);
			mData = nullptr;
		}
	}

	MessageType Message::fromIotaTransactionType(uint32_t iotaTransactionType)
	{
		switch (iotaTransactionType) {
		case 1: return MESSAGE_TYPE_MILESTONE;
		case 2: return MESSAGE_TYPE_TRANSACTION;
		default: return MESSAGE_TYPE_MAX;
		}
	}

	bool Message::loadFromJson(Document& json)
	{
		if (!json.IsObject()) {
			return false;
		}
		try {
			Value& data = json["data"];
			Value& payload = data["payload"];
			uint32_t iotaType = payload["type"].GetUint();
			auto type = fromIotaTransactionType(iotaType);
			mType = type;

			auto parentMessageIds = data["parentMessageIds"].GetArray();
			mParentMessageIds.reserve(parentMessageIds.Size());
			for (auto it = parentMessageIds.Begin(); it != parentMessageIds.End(); ++it) {
				iota::MessageId messageId;
				messageId.fromHex(it->GetString());
				mParentMessageIds.push_back(messageId);
			}

			if (MESSAGE_TYPE_MILESTONE == type) {
				mMilestoneId = payload["index"].GetInt();
				mMilestoneTimestamp = payload["timestamp"].GetInt64();
			}
			else if (MESSAGE_TYPE_TRANSACTION == type) {
				mData = DataTypeConverter::hexToBin(payload["data"].GetString());
				mIndex = convertHexToBin(payload["index"].GetString());
				std::string group = getGradidoGroupAlias();
				if (group != "") {
					printf("[iota::Message::loadFromJson] find gradido transaction with group alias: %s %s\n", group.data(), mId.toHex().data());
				}
			}
			mFetched = true;
		}
		catch (std::exception& e) {
			throw new std::runtime_error("iota milestone message result changed!");
		}
		return true;
	}

	bool Message::requestFromIota()
	{
		// GET /api/v1/messages/{messageId} 
		std::stringstream ss;
		ss << "messages/" << mId.toHex();

		Document& json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
		if (!json.IsObject()) {
			return false;
		}
		return loadFromJson(json);
	}

	bool Message::isGradidoTransaction() const
	{
		return getGradidoGroupAlias() != "";		
	}

	std::string Message::getGradidoGroupAlias() const
	{
		auto findPos = mIndex.find("GRADIDO");
		
		if (findPos != mIndex.npos) {
			printf("index: %s\n", mIndex.data());
			return mIndex.substr(8);
		}
		return "";
	}

	// ******************* Message Loader ********************************
	ConfirmedMessageLoader::ConfirmedMessageLoader(Poco::SharedPtr<Message> rootMilestone, uint8_t targetRecursionDeep/* = 0*/, uint8_t recursionDeep /*= 0 */)
		: UniLib::controller::CPUTask(ServerGlobals::g_IotaRequestCPUScheduler), mRootMilestone(rootMilestone), 
		mTargetRecursionDeep(targetRecursionDeep), mRecursionDeep(recursionDeep)
	{
		OrderingManager::getInstance()->pushMilestoneTaskObserver(rootMilestone->getMilestoneId(), rootMilestone->getMilestoneTimestamp());
	}
	ConfirmedMessageLoader::ConfirmedMessageLoader(MessageId id, Poco::SharedPtr<Message> rootMilestone, uint8_t targetRecursionDeep/* = 0*/, uint8_t recursionDeep /*= 0 */)
		: UniLib::controller::CPUTask(ServerGlobals::g_IotaRequestCPUScheduler), mMessage(new Message(id)), mRootMilestone(rootMilestone),
		mTargetRecursionDeep(targetRecursionDeep), mRecursionDeep(recursionDeep)
	{
		assert(!id.isEmpty());
		OrderingManager::getInstance()->pushMilestoneTaskObserver(rootMilestone->getMilestoneId(), rootMilestone->getMilestoneTimestamp());
	}
	
	ConfirmedMessageLoader::~ConfirmedMessageLoader()
	{
		OrderingManager::getInstance()->popMilestoneTaskObserver(mRootMilestone->getMilestoneId());
	}

	int ConfirmedMessageLoader::run()
	{
		if (!mMessage.isNull()) {
			if (!mMessage->isFetched()) {
				auto result = mMessage->requestFromIota();
				if (!result) {
					return -1;
				}
			}
		}
		else {
			mMessage = mRootMilestone;
		}
		
		if (MESSAGE_TYPE_MILESTONE == mMessage->getType() && mMessage->getMilestoneId() != mRootMilestone->getMilestoneId()) {
			// we found another milestone
			//printf("[%d] found milestone %d on recursion: %d\n", mRootMilestone->getMilestoneId(), mMessage->getMilestoneId(), mRecursionDeep);
			return 0;		
		}		
		else if (MESSAGE_TYPE_TRANSACTION == mMessage->getType()) {
			auto groupAlias = mMessage->getGradidoGroupAlias();
			// yes, we found a gradido transaction
			if (groupAlias != "") {
				// let make a gradido transaction from it and put it into ordering manager if it is from our group or it is a paired transaction from or for us
				Poco::AutoPtr<IotaMessageToTransactionTask> task = new IotaMessageToTransactionTask(
					mRootMilestone->getMilestoneId(), mRootMilestone->getMilestoneTimestamp(), mMessage
				);
				task->scheduleTask(task);
				printf("find transaction on deep: %d, root milestone: %d\n", mRecursionDeep, mRootMilestone->getMilestoneId());
			}
		}
				
		if (mTargetRecursionDeep > mRecursionDeep) {
			auto messageIds = mMessage->getParentMessageIds();
			int countExist = 0;
			auto cmCache = ConfirmedMessagesCache::getInstance();

			for (auto it = messageIds.begin(); it != messageIds.end(); it++) {
				// check if message was already checked, if not it will be added, if we go to next
				if (cmCache->existInCache(*it)) {
					countExist++;
					continue;
				}

				Poco::AutoPtr<ConfirmedMessageLoader> loader(new ConfirmedMessageLoader(*it, mRootMilestone, mTargetRecursionDeep, mRecursionDeep + 1));
				loader->scheduleTask(loader);
			}
			if (countExist == messageIds.size()) {
				//printf("[%d] exit on recursion %d because all message ids already exist\n", mRootMilestone->getMilestoneId(), mRecursionDeep);
			}
		}
		else {
			if (mTargetRecursionDeep > 1) {
				//printf("[%d] exit on recursion: %d\n", mRootMilestone->getMilestoneId(), mRecursionDeep);
			}
		}
		return 0;
	}
}