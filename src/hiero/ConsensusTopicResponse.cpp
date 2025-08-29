#include "ConsensusTopicResponse.h"
#include "gradido_blockchain/interaction/deserialize/TimestampRole.h"

using namespace gradido::interaction;

namespace hiero {
	ConsensusTopicResponse::ConsensusTopicResponse()
		: mMessage(0), mRunningHash(0), mSequenceNumber(0), mRunningHashVersion(0)
	{

	}

    ConsensusTopicResponse::ConsensusTopicResponse(const ConsensusTopicResponseMessage& message)
        : ConsensusTopicResponse()
    {
        mConsensusTimestamp = deserialize::TimestampRole(message["consensusTimestamp"_f].value());
        mMessage = std::make_shared<const memory::Block>(message["message"_f].value());
        if (message["runningHash"_f].has_value()) {
            mRunningHash = std::make_shared<const memory::Block>(message["runningHash"_f].value());
            mRunningHashVersion = message["runningHashVersion"_f].value();
        }
        if (message["sequenceNumber"_f].has_value()) {
            mSequenceNumber = message["sequenceNumber"_f].value();
        }
        if (message["chunkInfo"_f].has_value()) {
            mChunkInfo = ConsensusMessageChunkInfo(message["chunkInfo"_f].value());
        }
    }

    ConsensusTopicResponse::ConsensusTopicResponse(
        const gradido::data::Timestamp& consensusTimestamp,
        memory::ConstBlockPtr message,
        memory::ConstBlockPtr runningHash,
        uint64_t sequenceNumber,
        uint64_t runningHashVersion,
        const ConsensusMessageChunkInfo& chunkInfo
    ) :
        mConsensusTimestamp(consensusTimestamp),
        mMessage(message),
        mRunningHash(runningHash),
        mSequenceNumber(sequenceNumber),
        mRunningHashVersion(runningHashVersion),
        mChunkInfo(chunkInfo)
    {

    }

    ConsensusTopicResponse::~ConsensusTopicResponse()
    {

    }

    ConsensusTopicResponseMessage ConsensusTopicResponse::getMessage() const
    {
        ConsensusTopicResponseMessage message;
        message["consensusTimestamp"_f] = deserialize::TimestampMessage{
            mConsensusTimestamp.getSeconds(), mConsensusTimestamp.getNanos()
        };
        message["message"_f] = mMessage->copyAsVector();
        if (!mRunningHash->isEmpty()) {
            message["runningHash"_f] = mRunningHash->copyAsVector();
            message["runningHashVersion"_f] = mRunningHashVersion;
        }
        if (mSequenceNumber) {
            message["sequenceNumber"_f] = mSequenceNumber;
        }
        if (!mChunkInfo.empty()) {
            message["chunkInfo"_f] = mChunkInfo.getMessage();
        }
        return message;
    }
}