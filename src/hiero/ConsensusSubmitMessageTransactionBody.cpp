#include "ConsensusSubmitMessageTransactionBody.h"
#include "gradido_blockchain/interaction/serialize/HieroTopicIdRole.h"
#include "gradido_blockchain/interaction/deserialize/HieroTopicIdRole.h"

namespace hiero {
	ConsensusSubmitMessageTransactionBody::ConsensusSubmitMessageTransactionBody()
	{

	}
	ConsensusSubmitMessageTransactionBody::ConsensusSubmitMessageTransactionBody(
		const TopicId& topicId, 
		memory::ConstBlockPtr message, 
		const ConsensusMessageChunkInfo& chunkInfo
	) : mTopicId(topicId), mMessage(message), mChunkInfo(chunkInfo) {

	}
	ConsensusSubmitMessageTransactionBody::ConsensusSubmitMessageTransactionBody(const ConsensusSubmitMessageTransactionBodyMessage& message)
	{
		if (message["topicID"_f].has_value()) {
			mTopicId = gradido::interaction::deserialize::HieroTopicIdRole(message["topicID"_f].value());
		}
		if (message["message"_f].has_value()) {
			mMessage = std::make_shared<const memory::Block>(message["message"_f].value());
		}
		if (message["chunkInfo"_f].has_value()) {
			mChunkInfo = ConsensusMessageChunkInfo(message["chunkInfo"_f].value());
		}		
	}

	ConsensusSubmitMessageTransactionBodyMessage ConsensusSubmitMessageTransactionBody::getMessage() const
	{
		return ConsensusSubmitMessageTransactionBodyMessage{
			gradido::interaction::serialize::HieroTopicIdRole(mTopicId).getMessage(),
			mMessage->copyAsVector(),
			mChunkInfo.getMessage()
		};
	}
}