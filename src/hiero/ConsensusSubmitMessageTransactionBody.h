#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_SUBMIT_MESSAGE_TRANSACTION_BODY_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_SUBMIT_MESSAGE_TRANSACTION_BODY_H

#include "ConsensusMessageChunkInfo.h"
#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/data/hiero/TopicId.h"

namespace hiero {
    using ConsensusSubmitMessageTransactionBodyMessage = message<
        message_field<"topicID", 1, gradido::interaction::deserialize::HieroTopicIdMessage>,
        bytes_field<"message", 2>,
        message_field<"chunkInfo", 3, ConsensusMessageChunkInfoMessage>
    >;

    class ConsensusSubmitMessageTransactionBody 
    {
    public:
        ConsensusSubmitMessageTransactionBody();
        ConsensusSubmitMessageTransactionBody(const TopicId& topicId, memory::ConstBlockPtr message, const ConsensusMessageChunkInfo& chunkInfo);
        ConsensusSubmitMessageTransactionBody(const ConsensusSubmitMessageTransactionBodyMessage& message);

        ConsensusSubmitMessageTransactionBodyMessage getMessage() const;

        virtual ~ConsensusSubmitMessageTransactionBody() = default;

        // getter
        const TopicId& getTopicId() const { return mTopicId; }
        memory::ConstBlockPtr getMessageData() const { return mMessage; }
        const ConsensusMessageChunkInfo& getChunkInfo() const { return mChunkInfo; }

    protected:
        TopicId mTopicId;
        memory::ConstBlockPtr mMessage;
        ConsensusMessageChunkInfo mChunkInfo;
    };
}
#endif // __GRADIDO_NODE_HIERO_CONSENSUS_SUBMIT_MESSAGE_TRANSACTION_BODY_H