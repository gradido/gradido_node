#include "MessageListener.h"
#include "../../SingletonManager/SimpleOrderingManager.h"
#include "../../hiero/ConsensusTopicResponse.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "loguru/loguru.hpp"

using namespace hiero;

namespace client {
	namespace grpc {

		MessageListener::MessageListener(const hiero::TopicId& topicId, std::string_view communityId)
			: mTopicId(topicId), mCommunityId(communityId)
		{

		}

		MessageListener::~MessageListener()
		{

		}

		// move message binary
		void MessageListener::messageArrived(memory::Block&& consensusTopicResponseRaw)
		{
			auto result = message_coder<ConsensusTopicResponseMessage>::decode(consensusTopicResponseRaw.span());
			if (!result.has_value()) {
				LOG_F(WARNING, "invalid grpc response");
				return;
			}
			const auto& [message, bufferEnd2] = *result;
			processConsensusTopicResponse(message);
		}

		// will be called from grpc client if connection was closed
		void MessageListener::messagesStopped()
		{

		}

		void MessageListener::processConsensusTopicResponse(ConsensusTopicResponse&& response)
		{			
			LOG_F(1, "sequence number: %llu", response.getSequenceNumber());
			LOG_F(1, "consensus: %s", DataTypeConverter::timePointToString(response.getConsensusTimestamp().getAsTimepoint()).data());	
			
			hiero::TransactionId startTransactionId;
			if (!response.getChunkInfo().empty()) {
				auto total = response.getChunkInfo().getTotal();
				if (total > 1) {
					LOG_F(FATAL, "grpc Message contain more than one chunks: %d", total);
					throw GradidoNotImplementedException("Chunked Message processing");
				}
				startTransactionId = response.getChunkInfo().getInitialTransactionId();
			}
			SimpleOrderingManager::getInstance()->pushTransaction(std::move(response), mCommunityId);
		}

	}
}