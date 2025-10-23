#include "ConsensusGetTopicInfoQuery.h"
#include "gradido_blockchain/interaction/deserialize/HieroTopicIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroTopicIdRole.h"

using namespace gradido::interaction;

namespace hiero {
	ConsensusGetTopicInfoQuery::ConsensusGetTopicInfoQuery()
	{

	}

	ConsensusGetTopicInfoQuery::ConsensusGetTopicInfoQuery(const ConsensusGetTopicInfoQueryMessage& message)
		: mHeader(message["header"_f].value()), mTopicId(deserialize::HieroTopicIdRole(message["topicID"_f].value()))
	{
		
	}

	ConsensusGetTopicInfoQuery::ConsensusGetTopicInfoQuery(const QueryHeader& header, const TopicId& topicId)
		: mHeader(header), mTopicId(topicId)
	{

	}

	ConsensusGetTopicInfoQueryMessage ConsensusGetTopicInfoQuery::getMessage() const 
	{
		return ConsensusGetTopicInfoQueryMessage{
			mHeader.getMessage(),
			serialize::HieroTopicIdRole(mTopicId).getMessage()
		};
	}

	ConsensusGetTopicInfoQuery::~ConsensusGetTopicInfoQuery()
	{

	}
}