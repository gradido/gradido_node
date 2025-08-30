#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_RESPONSE_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_RESPONSE_H

#include "ConsensusMessageChunkInfo.h"
#include "gradido_blockchain/data/Timestamp.h"

namespace hiero {

    using ConsensusTopicResponseMessage = message<
        message_field<"consensusTimestamp", 1, gradido::interaction::deserialize::TimestampMessage>,
        bytes_field<"message", 2>,
        bytes_field<"runningHash", 3>,
        uint64_field<"sequenceNumber", 4>,
        uint64_field<"runningHashVersion", 5>,
        message_field<"chunkInfo", 6, ConsensusMessageChunkInfoMessage>
    >;

	class ConsensusTopicResponse
	{
	public:
		ConsensusTopicResponse();
        ConsensusTopicResponse(const ConsensusTopicResponseMessage& message);
        ConsensusTopicResponse(
            const gradido::data::Timestamp& consensusTimestamp,
            memory::ConstBlockPtr message,
            memory::ConstBlockPtr runningHash,
            uint64_t sequenceNumber,
            uint64_t runningHashVersion,
            const ConsensusMessageChunkInfo& chunkInfo
        );
		~ConsensusTopicResponse();

        ConsensusTopicResponseMessage getMessage() const;

        inline const gradido::data::Timestamp& getConsensusTimestamp() const { return mConsensusTimestamp; }
        inline memory::ConstBlockPtr getMessageData() const { return mMessage; }
        inline memory::ConstBlockPtr getRunningHash() const { return mRunningHash; }
        inline uint64_t getSequenceNumber() const { return mSequenceNumber; }
        inline uint64_t getRunningHashVersion() const { return mRunningHashVersion; }
        inline const ConsensusMessageChunkInfo& getChunkInfo() const { return mChunkInfo; }
        inline bool isMessageSame(const ConsensusTopicResponse& other) { return mMessage->isTheSame(other.mMessage); }
    protected:
        gradido::data::Timestamp mConsensusTimestamp;
        memory::ConstBlockPtr mMessage;
        memory::ConstBlockPtr mRunningHash;
        uint64_t mSequenceNumber;
        uint64_t mRunningHashVersion;
        ConsensusMessageChunkInfo mChunkInfo;
	};
}

#endif //__GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_RESPONSE_H