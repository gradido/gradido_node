#ifndef __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H
#define __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H

#include "../client/hiero/MessageObserver.h"
#include "ConsensusTopicResponse.h"

#include "gradido_blockchain/data/hiero/TopicId.h"

#include <atomic>

namespace hiero {
		
	class MessageListener : public client::hiero::MessageObserver<ConsensusTopicResponseMessage>
	{
	public:
		explicit MessageListener(const TopicId& topicId, std::string_view communityId);
		~MessageListener();

		// move message binary
		void onMessageArrived(const ConsensusTopicResponseMessage& consensusTopicResponse) override;

		// will be called from grpc client if connection was closed
		void onConnectionClosed() override;

		inline bool isClosed() const { return mIsClosed; }
		inline void cancelConnection() { mClientContext.TryCancel(); }
	protected:
		TopicId mTopicId;
		std::string mCommunityId;
		std::atomic<bool> mIsClosed;
	};
}

#endif //__GRADIDO_NODE_HIERO_MESSAGE_LISTENER_H