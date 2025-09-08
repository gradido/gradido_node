#include "SyncTopicOnStartup.h"
#include "../controller/SimpleOrderingManager.h"
#include "../blockchain/FileBased.h"
#include "../client/hiero/MirrorClient.h"
#include "../ServerGlobals.h"

#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "loguru/loguru.hpp"

using namespace gradido;
using namespace blockchain;
using namespace interaction;

namespace task {
	SyncTopicOnStartup::SyncTopicOnStartup(
		uint64_t lastKnownSequenceNumber,
		hiero::TopicId lastKnowTopicId,
		std::shared_ptr<gradido::blockchain::FileBased> blockchain
	) : CPUTaskGRPCReactor(ServerGlobals::g_CPUScheduler),
		mLastKnownSequenceNumber(lastKnownSequenceNumber),
		mLastKnowTopicId(lastKnowTopicId),
		mBlockchain(blockchain)
	{

	}

	SyncTopicOnStartup::~SyncTopicOnStartup()
	{

	}

	int SyncTopicOnStartup::run()
	{
		// check topic info
		const auto& topicInfo = mObject;
		const auto& mirrorNode = ServerGlobals::g_HieroMirrorNode;
		const auto& orderingManager = mBlockchain->getOrderingManager();

		// TODO: in future, check with other gradido nodes and use advanced strategy on failure
		// for example: check previous transaction until one was found,
		// or check if maybe or block files get corrupted
		auto lastTransactionIdentical = checkLastTransaction();

		// start ordering manager
		orderingManager->init(mLastKnownSequenceNumber);
		
		// load transactions from mirror node
		auto newTransactionCountPreviousTopic = loadTransactionsFromMirrorNode(mLastKnowTopicId, mLastKnownSequenceNumber + 1);
		auto newTransactionCountCurrentTopic = 0;

		// was there a topic reset after our last run?
		if (!topicInfo.getTopicId().empty() && topicInfo.getTopicId() != mLastKnowTopicId) {
			// load transactions from mirror node
			newTransactionCountCurrentTopic = loadTransactionsFromMirrorNode(topicInfo.getTopicId(), 0);
			VLOG_SCOPE_F(0, "topic reset since our last run");
			LOG_F(INFO, "communityId: %s, old topic id: %s, new topic id: %s",
				mBlockchain->getCommunityId().data(),
				mLastKnowTopicId.toString().data(),
				topicInfo.getTopicId().toString().data()
			);
			LOG_F(INFO, "transaction from old topic: %u, from new topic: %u",
				newTransactionCountPreviousTopic,
				newTransactionCountCurrentTopic
			);
		}
		else {
			LOG_F(INFO, "%s added transactions: %u", mBlockchain->getCommunityId().data(), newTransactionCountPreviousTopic);
		}

		// start listening
		mBlockchain->startListening();
		return 0;
	}

	bool SyncTopicOnStartup::checkLastTransaction() const
	{
		const auto& mirrorNode = ServerGlobals::g_HieroMirrorNode;

		// check last transaction
		auto lastTransaction = mBlockchain->findOne(Filter::LAST_TRANSACTION);
		if (!lastTransaction) {
			// blockchain is empty
			return false;
		}
		
		auto lastTransactionMirrorRaw = mirrorNode->getTopicMessageBySequenceNumber(mLastKnowTopicId, mLastKnownSequenceNumber).getMessageData();
		if (!lastTransactionMirrorRaw) {
			LOG_F(
				WARNING, 
				"cannot find last transaction (any more) in hiero network. CommunityId: %s, lastKnownTopicId: %s, sequenceNumber: %llu, transactionNr: %llu, confirmedAt: %s",
				mBlockchain->getCommunityId().data(), 
				mLastKnowTopicId.toString().data(),
				mLastKnownSequenceNumber, 
				lastTransaction->getTransactionNr(),
				DataTypeConverter::timePointToString(lastTransaction->getConfirmedTransaction()->getConfirmedAt().getAsTimepoint())
			);
			return false;
		}

		deserialize::Context deserializer(lastTransactionMirrorRaw);
		deserializer.run();
		if (!deserializer.isGradidoTransaction()) {
			LOG_F(
				ERROR, 
				"last transaction on mirrors is invalid Gradido Transaction. CommunityId: %s, lastKnownTopicId: %s, sequenceNumber: %llu, transactionNr: %llu, confirmedAt: %s",
				mBlockchain->getCommunityId().data(),
				mLastKnowTopicId.toString().data(),
				mLastKnownSequenceNumber,
				lastTransaction->getTransactionNr(),
				DataTypeConverter::timePointToString(lastTransaction->getConfirmedTransaction()->getConfirmedAt().getAsTimepoint())
			);
			return false;
		}
		auto lastTransactionMirror = deserializer.getGradidoTransaction();
		if (!lastTransaction->getConfirmedTransaction()->getGradidoTransaction()->getFingerprint()->isTheSame(lastTransactionMirror->getFingerprint())) {
			VLOG_SCOPE_F(-2, "last transaction on mirror is different");
			toJson::Context lastTransactionJson(*lastTransaction->getConfirmedTransaction());
			toJson::Context lastTransactionMirrorJson(*lastTransactionMirror);
			bool prettyJson = true;
			LOG_F(ERROR, "own transaction: %s", lastTransactionJson.run(prettyJson).data());
			LOG_F(
				ERROR, 
				"from topic: %s, sequenceNumber: %llu: %s", 
				mLastKnowTopicId.toString().data(), 
				mLastKnownSequenceNumber, 
				lastTransactionMirrorJson.run(prettyJson).data()
			);
			return false;
		}
		return true;
	}

	uint32_t SyncTopicOnStartup::loadTransactionsFromMirrorNode(hiero::TopicId topicId, uint64_t startSequenceNumber)
	{
		const auto& mirrorNode = ServerGlobals::g_HieroMirrorNode;
		const auto& orderingManager = mBlockchain->getOrderingManager();

		uint32_t limit = MAGIC_NUMBER_MIRROR_API_GET_TOPIC_MESSAGES_BULK_SIZE;
		uint32_t responsesCount = 0;
		uint32_t addedTransactionsSum = 0;
		uint64_t sequenceNumber = startSequenceNumber;
		do {
			// load new transactions from mirror node
			auto responses = mirrorNode->listTopicMessagesById(topicId, limit, sequenceNumber, "asc");
			if (responses.empty()) break;
			sequenceNumber += responses.size();
			responsesCount = responses.size();
			addedTransactionsSum += responsesCount;
			for (auto& response : responses) {
				orderingManager->pushTransaction(std::move(response));
			}
		} while (responsesCount == limit);

		return addedTransactionsSum;
	}
}