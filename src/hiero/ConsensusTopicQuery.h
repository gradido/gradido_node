#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_QUERY_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_QUERY_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/Timestamp.h"

namespace hiero {
    using ConsensusTopicQueryMessage = message<
        message_field<"topicID", 1, gradido::interaction::deserialize::HieroTopicIdMessage>,
        message_field<"consensusStartTime", 2, gradido::interaction::deserialize::TimestampMessage>,
        message_field<"consensusEndTime", 3, gradido::interaction::deserialize::TimestampMessage>,
        uint64_field<"limit", 4>
    >;

    class ConsensusTopicQuery {
    public:
        ConsensusTopicQuery();
        ConsensusTopicQuery(const ConsensusTopicQueryMessage& message);

        //! \param topicId A required topic ID to retrieve messages for.
        //! \param consensusStartTime Include messages which reached consensus on or after this time. Defaults to current time if not set.
        //! \param consensusEndTime Include messages which reached consensus before this time. If not set it will receive indefinitely.
        //! \param limit The maximum number of messages to receive before stopping. If not set or set to zero it will return messages indefinitely.
        ConsensusTopicQuery(
            const TopicId& topicId, 
            const gradido::data::Timestamp& consensusStartTime = gradido::data::Timestamp(0, 0),
            const gradido::data::Timestamp& consensusEndTime = gradido::data::Timestamp(0, 0),
            uint64_t limit = 0
        );
        ~ConsensusTopicQuery();

        ConsensusTopicQueryMessage getMessage() const;

        void setTopicId(const TopicId& topicId) { mTopicId = topicId; }
        void setConsensusStartTime(const gradido::data::Timestamp& consensusStartTime) { mConsensusStartTime = consensusStartTime; }
        void setConsensusEndTime(const gradido::data::Timestamp& consensusEndTime) { mConsensusEndTime = consensusEndTime; }
        void setLimit(uint64_t limit) { mLimit = limit; }

        inline const TopicId& getTopicId() const { return mTopicId; }
        inline const gradido::data::Timestamp& getConsensusStartTime() const { return mConsensusStartTime; }
        inline const gradido::data::Timestamp& getConsensusEndTime() const { return mConsensusEndTime; }
        inline uint64_t getLimit() const { return mLimit; }

        // protobuf 
        // varint for uint64_t, 0 - 10 Bytes
        // varint for uint32_t, 0 - 5 Bytes
        // 1 Byte for Tag <= 15 (x11)
        // 1 Byte for size for embedded message (x3)
        // size: 6 x 10 Bytes + 2 x 5 Bytes + 11 + 3
        static constexpr size_t maxSize() { return 6 * 10 + 2 * 5 + 11 + 3; }

    protected:
        TopicId mTopicId;
        gradido::data::Timestamp mConsensusStartTime;
        gradido::data::Timestamp mConsensusEndTime;
        uint64_t mLimit;
    };
}

#endif // __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_QUERY_H