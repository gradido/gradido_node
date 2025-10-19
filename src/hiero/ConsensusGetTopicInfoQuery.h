#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_QUERY_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_QUERY_H

#include "ConsensusTopicQuery.h"
#include "QueryHeader.h"

namespace hiero {
	using ConsensusGetTopicInfoQueryMessage = message<
		message_field<"header", 1, QueryHeaderMessage>,
		message_field<"topicID", 2, gradido::interaction::deserialize::HieroTopicIdMessage>
	>;

	class ConsensusGetTopicInfoQuery : public ConsensusTopicQuery
	{
	public:
		ConsensusGetTopicInfoQuery();
		ConsensusGetTopicInfoQuery(const ConsensusGetTopicInfoQueryMessage& message);
		ConsensusGetTopicInfoQuery(const QueryHeader& header, const TopicId& topicId);
		~ConsensusGetTopicInfoQuery();

		ConsensusGetTopicInfoQueryMessage getMessage() const;

		inline const QueryHeader& getHeader() const { return mHeader; }
		inline const TopicId& getTopicId() const { return mTopicId; }
	private:
		QueryHeader mHeader;
		TopicId mTopicId;
	};
}
#endif //__GRADIDO_NODE_HIERO_CONSENSUS_GET_TOPIC_INFO_QUERY_H