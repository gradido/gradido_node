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
		mConfirmedAtLastReadedTransaction = data::Timestamp();
		if (LastTransactionState::IDENTICAL == lastTransactionIdentical) {
			mConfirmedAtLastReadedTransaction = mBlockchain->findOne(Filter::LAST_TRANSACTION)->getConfirmedTransaction()->getConfirmedAt();
		}
		auto newTransactionCountPreviousTopic = loadTransactionsFromMirrorNode(mLastKnowTopicId);
		auto newTransactionCountCurrentTopic = 0;

		// was there a topic reset after our last run?
		if (!topicInfo.getTopicId().empty() && topicInfo.getTopicId() != mLastKnowTopicId) {
			// load transactions from mirror node
			newTransactionCountCurrentTopic = loadTransactionsFromMirrorNode(topicInfo.getTopicId());
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
		mBlockchain->startListening(mConfirmedAtLastReadedTransaction);
		return 0;
	}

	SyncTopicOnStartup::LastTransactionState SyncTopicOnStartup::checkLastTransaction()
	{
		const auto& mirrorNode = ServerGlobals::g_HieroMirrorNode;

		// check last transaction
		auto lastTransaction = mBlockchain->findOne(Filter::LAST_TRANSACTION);
		if (!lastTransaction) {
			// blockchain is empty
			return LastTransactionState::NONE;
		}
		auto confirmedAt = lastTransaction->getConfirmedTransaction()->getConfirmedAt();
		auto transactionFromMirror = mirrorNode->getTopicMessageByConsensusTimestamp(confirmedAt);
		auto lastTransactionMirrorRaw = transactionFromMirror.getMessageData();
		if (!lastTransactionMirrorRaw) {
			LOG_F(
				WARNING,
				"cannot find last transaction (any more) in hiero network. CommunityId: %s, lastKnownTopicId: %s, sequenceNumber: %llu, transactionNr: %llu, confirmedAt: %s",
				mBlockchain->getCommunityId().data(),
				mLastKnowTopicId.toString().data(),
				mLastKnownSequenceNumber,
				lastTransaction->getTransactionNr(),
				confirmedAt.toString().data()
			);
			return LastTransactionState::NOT_FOUND;
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
				lastTransaction->getConfirmedTransaction()->getConfirmedAt().toString().data()
			);
			return LastTransactionState::INVALID;
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
			return LastTransactionState::NOT_IDENTICAL;
		}
		mLastKnownSequenceNumber = transactionFromMirror.getSequenceNumber();
		return LastTransactionState::IDENTICAL;
	}

	uint32_t SyncTopicOnStartup::loadTransactionsFromMirrorNode(hiero::TopicId topicId)
	{
		const auto& mirrorNode = ServerGlobals::g_HieroMirrorNode;
		const auto& orderingManager = mBlockchain->getOrderingManager();

		uint32_t limit = MAGIC_NUMBER_MIRROR_API_GET_TOPIC_MESSAGES_BULK_SIZE;
		uint32_t responsesCount = 0;
		uint32_t addedTransactionsSum = 0;
		do {
			// load new transactions from mirror node
			auto responses = mirrorNode->listTopicMessagesById(topicId, mConfirmedAtLastReadedTransaction, limit, "asc");
			if (responses.empty()) break;
			responsesCount = responses.size();
			mConfirmedAtLastReadedTransaction = responses.back().getConsensusTimestamp();
			addedTransactionsSum += responsesCount;
			for (auto& response : responses) {
				orderingManager->pushTransaction(std::move(response));
			}
		} while (responsesCount == limit);

		return addedTransactionsSum;
	}
}