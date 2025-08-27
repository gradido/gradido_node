#ifndef __GRADIDO_NODE_CLIENT_GRPC_H
#define __GRADIDO_NODE_CLIENT_GRPC_H

#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "IMessageObserver.h"
#include "TopicMessageQueryReactor.h"

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

			//! will use ConsensusTopicQuery from hiero/hedera consensus_service.proto
			//! \param IMessageObserver handler for messages, will use TopicMessageQueryReactor for translate from grpc to gradido_node 
			//! \param topicId A required topic ID to retrieve messages for.
			//! \param consensusStartTime Include messages which reached consensus on or after this time. Defaults to current time if not set.
			//! \param consensusEndTime Include messages which reached consensus before this time. If not set it will receive indefinitely.
			//! \param limit The maximum number of messages to receive before stopping. If not set or set to zero it will return messages indefinitely.
			void subscribeTopic(
				IMessageObserver* handler,
				const hiero::TopicId& topicId,
				gradido::data::Timestamp consensusStartTime = gradido::data::Timestamp(),
				gradido::data::Timestamp consensusEndTime = gradido::data::Timestamp(),
				uint64_t limit = 0				
			);

		protected:
			Client(std::shared_ptr<::grpc::Channel> channel);

			memory::Block serializeConsensusTopicQuery(
				const hiero::TopicId& topicId,
				const gradido::data::Timestamp& consensusStartTime,
				const gradido::data::Timestamp& consensusEndTime,
				uint64_t limit
			);
			// grpc 
			std::unique_ptr<::grpc::GenericStub> mGenericStub;
		};
	}
}

#endif // __GRADIDO_NODE_CLIENT_GRPC_H