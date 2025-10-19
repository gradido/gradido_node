#include "ConsensusTopicQuery.h"
#include "gradido_blockchain/interaction/serialize/HieroTopicIdRole.h"
#include "gradido_blockchain/interaction/deserialize/HieroTopicIdRole.h"
#include "gradido_blockchain/interaction/deserialize/TimestampRole.h"

using namespace gradido::interaction;

namespace hiero {
    ConsensusTopicQuery::ConsensusTopicQuery() {
        
    }

    ConsensusTopicQuery::ConsensusTopicQuery(const ConsensusTopicQueryMessage& message) 
        : mTopicId(deserialize::HieroTopicIdRole(message["topicID"_f].value())),
        mConsensusStartTime(deserialize::TimestampRole(message["consensusStartTime"_f].value())),
        mConsensusEndTime(deserialize::TimestampRole(message["consensusEndTime"_f].value())),
        mLimit(message["limit"_f].value())
    {
    }

    ConsensusTopicQuery::ConsensusTopicQuery(
        const TopicId& topicId, 
        const gradido::data::Timestamp& consensusStartTime, 
        const gradido::data::Timestamp& consensusEndTime, 
        uint64_t limit
    ) : mTopicId(topicId), mConsensusStartTime(consensusStartTime), mConsensusEndTime(consensusEndTime), mLimit(limit) {
        
    }

    ConsensusTopicQuery::~ConsensusTopicQuery() {
        
    }

    ConsensusTopicQueryMessage ConsensusTopicQuery::getMessage() const {
        return ConsensusTopicQueryMessage(
            serialize::HieroTopicIdRole(mTopicId).getMessage(),
            deserialize::TimestampMessage{ mConsensusStartTime.getSeconds(), mConsensusStartTime.getNanos() },
            deserialize::TimestampMessage{ mConsensusEndTime.getSeconds(), mConsensusEndTime.getNanos() },
            mLimit
        );
    }
}