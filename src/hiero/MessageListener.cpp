
#include "MessageListener.h"
#include "../controller/SimpleOrderingManager.h"
#include "../blockchain/FileBasedProvider.h"
#include "ConsensusTopicResponse.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "loguru/loguru.hpp"

namespace hiero {

	MessageListener::MessageListener(const TopicId& topicId, std::string_view communityId)
		: mTopicId(topicId), mCommunityId(communityId), mIsClosed(false)
	{

	}

	MessageListener::~MessageListener()
	{

	}

	// move message binary
	void MessageListener::onMessageArrived(const ConsensusTopicResponseMessage& consensusTopicResponse)
	{
		ConsensusTopicResponse response(consensusTopicResponse);

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
	void MessageListener::onConnectionClosed()
	{		
		mIsClosed = true;
		LOG_F(WARNING, "connection closed on topic: %s", mTopicId.toString().data());
	}
}