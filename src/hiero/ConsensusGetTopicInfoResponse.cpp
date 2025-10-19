#include "ConsensusGetTopicInfoResponse.h"
#include "gradido_blockchain/interaction/deserialize/HieroTopicIdRole.h"

using namespace gradido::interaction;

namespace hiero {
	ConsensusGetTopicInfoResponse::ConsensusGetTopicInfoResponse()
	{

	}
	ConsensusGetTopicInfoResponse::ConsensusGetTopicInfoResponse(const ConsensusGetTopicInfoResponseMessage& message)
	{
		if (message["header"_f].has_value()) {
			mResponseHeader = ResponseHeader(message["header"_f].value());
		}
		if (message["topicID"_f].has_value()) {
			mTopicId = deserialize::HieroTopicIdRole(message["topicID"_f].value());
		}		
		if (message["topicInfo"_f].has_value()) {
			mTopicInfo = ConsensusTopicInfo(message["topicInfo"_f].value());
		}
	}

	ConsensusGetTopicInfoResponse::~ConsensusGetTopicInfoResponse()
	{

	}
}