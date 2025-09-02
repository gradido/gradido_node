#ifndef __GRADIDO_NODE_CLIENT_GRPC_CLIENT_H
#define __GRADIDO_NODE_CLIENT_GRPC_CLIENT_H

#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "MessageObserver.h"
#include "../../hiero/TransactionGetReceiptResponse.h"
#include "../../hiero/ConsensusTopicQuery.h"
#include "../../hiero/ConsensusTopicResponse.h"
#include "../../hiero/Query.h"

#include <string>
#include <memory>
#include <map>

namespace grpc {
	class Channel;
}

namespace client {
	namespace grpc {

		class Client
		{
		public:
			~Client() = default;

			// throw on error
			//! \param targetUrl full url with port and protocol
			static std::shared_ptr<Client> createForTarget(const std::string& targetUrl);

			//! \param MessageObserver handler for messages, will use SubscriptionReactor for translate from grpc to gradido_node 
			void subscribeTopic(
				const hiero::ConsensusTopicQuery& request,
				std::shared_ptr<MessageObserver<hiero::ConsensusTopicResponseMessage>> responseListener
			);

			void getTransactionReceipts(
				const hiero::TransactionId& request,
				std::shared_ptr<MessageObserver<hiero::TransactionGetReceiptResponseMessage>> responseListener
			);			

			inline std::shared_ptr<::grpc::Channel> getChannel() { return mChannel; }

		protected:
			Client(std::shared_ptr<::grpc::Channel> channel);

			memory::Block serializeConsensusTopicQuery(const hiero::ConsensusTopicQuery& consensusTopicQuery);
			memory::Block serializeQuery(const hiero::Query& query);
			// grpc 
			std::shared_ptr<::grpc::Channel> mChannel;
		};
	}
}

#endif // __GRADIDO_NODE_CLIENT_GRPC_CLIENT_H