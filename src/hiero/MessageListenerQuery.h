#ifndef __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_QUERY_H
#define __GRADIDO_NODE_HIERO_MESSAGE_LISTENER_QUERY_H

#include "../client/hiero/TopicMessageQuery.h"
#include "ConsensusTopicResponse.h"

#include "gradido_blockchain/data/hiero/TopicId.h"

#include <atomic>

namespace hiero {
		
	class MessageListenerQuery : public client::hiero::TopicMessageQuery
	{
	public:
		explicit MessageListenerQuery(const TopicId& topicId, std::string_view communityId, ConsensusTopicQuery startQuery);
		~MessageListenerQuery();

		// move message binary
		void onMessageArrived(ConsensusTopicResponse&& consensusTopicResponse) override;

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

#endif //__GRADIDO_NODE_HIERO_MESSAGE_LISTENER_QUERY_H