
#include "MessageListenerQuery.h"
#include "../blockchain/FileBasedProvider.h"
#include "../client/hiero/ConsensusClient.h"
#include "../controller/SimpleOrderingManager.h"
#include "../task/SyncTopic.h"
#include "ConsensusTopicResponse.h"

#include "gradido_blockchain/Application.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "loguru/loguru.hpp"

using client::hiero::ConnectionClosedReason;

namespace hiero {

	MessageListenerQuery::MessageListenerQuery(const TopicId& topicId, std::string_view communityId, ConsensusTopicQuery startQuery)
		: client::hiero::TopicMessageQuery("MsgLstQry", startQuery), mTopicId(topicId), mCommunityId(communityId), mIsClosed(false)
	{

	}

	MessageListenerQuery::~MessageListenerQuery()
	{

	}

	// move message binary
	void MessageListenerQuery::onMessageArrived(ConsensusTopicResponse&& response)
	{
		// hiero::TransactionId startTransactionId;
		if (!response.getChunkInfo().empty()) {
			auto total = response.getChunkInfo().getTotal();
			if (total > 1) {
				LOG_F(FATAL, "grpc Message contain more than one chunks: %d", total);
				throw GradidoNotImplementedException("Chunked Message processing");
			}
			// startTransactionId = response.getChunkInfo().getInitialTransactionId();
		}
		auto blockchain = gradido::blockchain::FileBasedProvider::getInstance()->findBlockchain(mCommunityId);
		auto fileBasedBlockchain = static_cast<gradido::blockchain::FileBased*>(blockchain.get());

		fileBasedBlockchain->getOrderingManager()->pushTransaction(std::move(response));
	}

	// will be called from grpc client if connection was closed
	void MessageListenerQuery::onConnectionClosed(ConnectionClosedReason reason) noexcept
	{
		//mIsClosed = true;
		if (ConnectionClosedReason::Reconnect == reason) {
			LOG_F(WARNING, "connection closed on topic: %s, try reconnect", mTopicId.toString().data());
		}
		else {
			if (!Application::getStopToken().stop_requested()) {
				auto blockchain = gradido::blockchain::FileBasedProvider::getInstance()->findBlockchain(mCommunityId);
				auto fileBasedBlockchain = static_cast<gradido::blockchain::FileBased*>(blockchain.get());
				auto task = fileBasedBlockchain->getTopicSyncTask();
				auto hieroClient = fileBasedBlockchain->pickHieroClient();
				hieroClient->getTopicInfo(fileBasedBlockchain->getHieroTopicId(), task);
				task->scheduleTask(task);
			}
		}
	}
}