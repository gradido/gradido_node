#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_RESPONSE_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_RESPONSE_H

#include "ConsensusTopicInfo.h"
#include "ResponseHeader.h"
#include "gradido_blockchain/data/hiero/TopicId.h"

namespace hiero {
	using ConsensusGetTopicInfoResponseMessage = message<
		message_field<"header", 1, ResponseHeaderMessage>,
		message_field<"topicID", 2, gradido::interaction::deserialize::HieroTopicIdMessage>,
		message_field<"topicInfo", 5, ConsensusTopicInfoMessage>
	>;


	class ConsensusGetTopicInfoResponse
	{
	public:
		ConsensusGetTopicInfoResponse();
		ConsensusGetTopicInfoResponse(const ConsensusGetTopicInfoResponseMessage& topicInfo);
		~ConsensusGetTopicInfoResponse();

		const ResponseHeader& getResponseHeader() const { return mResponseHeader; }
		const TopicId& getTopicId() const { return mTopicId; }
		const ConsensusTopicInfo& getTopicInfo() const { return mTopicInfo; }
	private:
		ResponseHeader mResponseHeader;
		TopicId mTopicId;
		ConsensusTopicInfo mTopicInfo;
	};
}
#endif //__GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_RESPONSE_H