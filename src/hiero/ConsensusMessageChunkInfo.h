#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_MESSAGE_CHUNK_INFO_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_MESSAGE_CHUNK_INFO_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "rapidjson/document.h"

namespace hiero {
	using ConsensusMessageChunkInfoMessage = message<
		message_field<"initialTransactionID", 1, gradido::interaction::deserialize::HieroTransactionIdMessage>, // The TransactionID of the first chunk.
		int32_field<"total", 2>, // The total number of chunks in the message.
		int32_field<"number", 3> // The sequence number (from 1 to total) of the current chunk in the message.
	>;

	class ConsensusMessageChunkInfo
	{
	public:
		ConsensusMessageChunkInfo();
		ConsensusMessageChunkInfo(const ConsensusMessageChunkInfoMessage& message);
		ConsensusMessageChunkInfo(const TransactionId& initialTransactionId, int32_t total, int32_t number);
		ConsensusMessageChunkInfo(const rapidjson::Value& json);
		~ConsensusMessageChunkInfo();

		ConsensusMessageChunkInfoMessage getMessage() const;

		inline const TransactionId& getInitialTransactionId() const { return mInitialTransactionID; }
		inline const int32_t getTotal() const { return mTotal; }
		inline const int32_t getNumber() const { return mNumber; }

		inline bool empty() const { return mInitialTransactionID.empty(); }

	protected:
		TransactionId mInitialTransactionID;
		int32_t mTotal;
		int32_t mNumber;
	};
}

#endif //__GRADIDO_NODE_HIERO_CONSENSUS_MESSAGE_CHUNK_INFO_H