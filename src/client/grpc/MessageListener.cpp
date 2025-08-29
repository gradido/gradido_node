#include "MessageListener.h"
#include "../../SingletonManager/SimpleOrderingManager.h"
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/deserialize/HieroTransactionIdRole.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "loguru/loguru.hpp"

using namespace gradido::interaction::deserialize;

using ConsensusMessageChunkInfoMessage = message<
	message_field<"initialTransactionID", 1, HieroTransactionIdMessage>, // The TransactionID of the first chunk.
	int32_field<"total", 2>, // The total number of chunks in the message.
	int32_field<"number", 3> // The sequence number (from 1 to total) of the current chunk in the message.
>;

using ConsensusTopicResponseMessage = message<
	message_field<"consensusTimestamp", 1, TimestampMessage>,
	bytes_field<"message", 2>,
	bytes_field<"runningHash", 3>,
	uint64_field<"sequenceNumber", 4>,
	uint64_field<"runningHashVersion", 5>,
	message_field<"chunkInfo", 6, ConsensusMessageChunkInfoMessage>
>;

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
		void MessageListener::messageArrived(memory::Block&& message)
		{

		}

		// will be called from grpc client if connection was closed
		void MessageListener::messagesStopped()
		{

		}

		void MessageListener::processConsensusTopicResponse(const memory::Block& raw)
		{			
			auto result = message_coder<ConsensusTopicResponseMessage>::decode(raw.span());
			if (!result.has_value()) {
				LOG_F(WARNING, "invalid grpc response");
				return;
			}
			const auto& [message, bufferEnd2] = *result;
			auto ownMessage = message["message"_f];
			auto runningHashMessage = message["runningHash"_f];
			auto consensusTimestampMessage = message["consensusTimestamp"_f].value();
			gradido::data::Timestamp consensusTimestamp(consensusTimestampMessage["seconds"_f].value(), consensusTimestampMessage["nanos"_f].value());
			std::shared_ptr<const gradido::data::GradidoTransaction> gradidoTransaction;
			memory::Block runningHash(0);
			LOG_F(1, "sequence number: %llu", message["sequenceNumber"_f].value());
			LOG_F(1, "consensus: %s", DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());

			if (ownMessage.has_value()) {
				auto ownMessageBlock = std::make_shared<memory::Block>(ownMessage.value());
				Context c(ownMessageBlock, Type::GRADIDO_TRANSACTION);
				c.run();
				if (c.isGradidoTransaction()) {
					gradidoTransaction = c.getGradidoTransaction();
				}
			}

			if (runningHashMessage.has_value()) {
				runningHash = memory::Block(runningHashMessage.value());
			}
			
			hiero::TransactionId startTransactionId;
			if (message["chunkInfo"_f].has_value()) {
				auto chunkInfoMessage = message["chunkInfo"_f].value();
				auto total = chunkInfoMessage["total"_f].value();
				if (total > 1) {
					LOG_F(FATAL, "grpc Message contain more than one chunks: %d", total);
					throw GradidoNotImplementedException("Chunked Message processing");
				}
				auto transactionIdMessage = chunkInfoMessage["initialTransactionID"_f].value();
				startTransactionId = HieroTransactionIdRole(transactionIdMessage);
			}
			SimpleOrderingManager::getInstance()->pushTransaction(
				std::move(runningHash),
				startTransactionId,
				message["sequenceNumber"_f].value(),
				std::make_shared<memory::Block>(ownMessage.value()),
				consensusTimestamp,
				mCommunityId
			);
		}

	}
}