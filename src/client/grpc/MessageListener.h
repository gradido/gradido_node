#ifndef __GRADIDO_NODE_CLIENT_GRPC_H
#define __GRADIDO_NODE_CLIENT_GRPC_H

#include "IMessageObserver.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/GradidoTransaction.h"

namespace hiero {
	class ConsensusTopicResponse;
};

namespace client {
	namespace grpc {
		class MessageListener : public IMessageObserver
		{
		public:
			MessageListener(const hiero::TopicId& topicId, std::string_view communityId);
			~MessageListener();

			// move message binary
			virtual void messageArrived(memory::Block&& consensusTopicResponseRaw);

			// will be called from grpc client if connection was closed
			virtual void messagesStopped();
		protected:
			void processConsensusTopicResponse(hiero::ConsensusTopicResponse&& response);
			hiero::TopicId mTopicId;
			std::string mCommunityId;
		};
	}
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_H