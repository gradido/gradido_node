#ifndef __GRADIDO_NODE_CLIENT_GRPC_H
#define __GRADIDO_NODE_CLIENT_GRPC_H

#include "IMessageObserver.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/GradidoTransaction.h"

namespace client {
	namespace grpc {
		class MessageListener : public IMessageObserver
		{
		public:
			MessageListener(const hiero::TopicId& topicId, std::string_view communityId);
			~MessageListener();

			// move message binary
			virtual void messageArrived(memory::Block&& message);

			// will be called from grpc client if connection was closed
			virtual void messagesStopped();
		protected:
			std::shared_ptr<const gradido::data::GradidoTransaction> processConsensusTopicResponse(const memory::Block& raw);
			hiero::TopicId mTopicId;
			std::string mCommunityId;
		};
	}
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_H