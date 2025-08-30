#ifndef __GRADIDO_NODE_CLIENT_GRPC_H
#define __GRADIDO_NODE_CLIENT_GRPC_H

#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "IMessageObserver.h"
#include "TopicMessageQueryReactor.h"
#include "../../hiero/TransactionGetReceiptResponse.h"
#include "../../hiero/ConsensusTopicQuery.h"
#include "../../hiero/Query.h"

#include <string>
#include <memory>
#include <map>

namespace grpc {
	class Channel;
	class ByteBuffer;
	template <typename Request, typename Response>
	class TemplatedGenericStub;
	using GenericStub = TemplatedGenericStub<grpc::ByteBuffer, grpc::ByteBuffer>;
}

namespace client {
	namespace grpc {
		class Client
		{
		public:
			~Client();

			// throw on error
			//! \param targetUrl full url with port and protocol
			static std::shared_ptr<Client> createForTarget(const std::string& targetUrl);

			//! \param IMessageObserver handler for messages, will use TopicMessageQueryReactor for translate from grpc to gradido_node 
			void subscribeTopic(IMessageObserver* handler, const hiero::ConsensusTopicQuery& consensusTopicQuery);

			hiero::TransactionGetReceiptResponse getTransactionReceipts(const hiero::TransactionId& transactionId);

		protected:
			Client(std::shared_ptr<::grpc::Channel> channel);

			memory::Block serializeConsensusTopicQuery(const hiero::ConsensusTopicQuery& consensusTopicQuery);
			memory::Block serializeQuery(const hiero::Query& query);
			// grpc 
			std::unique_ptr<::grpc::GenericStub> mGenericStub;
		};
	}
}

#endif // __GRADIDO_NODE_CLIENT_GRPC_H