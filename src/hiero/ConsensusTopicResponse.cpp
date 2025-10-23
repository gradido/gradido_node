#include "ConsensusTopicResponse.h"
#include "rapidjson.h"
#include "gradido_blockchain/interaction/deserialize/TimestampRole.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"

#include "rapidjson/document.h"

using namespace gradido::interaction;
using namespace rapidjson;
using namespace rapidjson_helper;

namespace hiero {
    static const char* className = "ConsensusTopicResponse";
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

    ConsensusTopicResponse::ConsensusTopicResponse(const Value& json)
        : ConsensusTopicResponse()
    {      
        checkMember(json, "consensus_timestamp", MemberType::STRING, className);
        checkMember(json, "message", MemberType::STRING, className);
        checkMember(json, "running_hash", MemberType::STRING, className);
        checkMember(json, "running_hash_version", MemberType::INTEGER, className);
        checkMember(json, "sequence_number", MemberType::INTEGER, className);
        mConsensusTimestamp = timestampFromJson(json["consensus_timestamp"]);
        mMessage = std::make_shared<const memory::Block>(memory::Block::fromBase64(
            std::string(json["message"].GetString(), json["message"].GetStringLength())
        ));
        mRunningHash = std::make_shared<const memory::Block>(memory::Block::fromBase64(
            std::string(json["running_hash"].GetString(), json["running_hash"].GetStringLength())
        ));
        mRunningHashVersion = json["running_hash_version"].GetInt();
        mSequenceNumber = json["sequence_number"].GetInt64();

        if (json.HasMember("chunk_info") && json["chunk_info"].IsObject()) {
            mChunkInfo = ConsensusMessageChunkInfo(json["chunk_info"]);
        }
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