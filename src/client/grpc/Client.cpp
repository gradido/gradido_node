#include "Client.h"
#include "Exceptions.h"
#include "gradido_blockchain/memory/VectorCacheAllocator.h"
#include "gradido_blockchain/interaction/serialize/Protopuf.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>



using namespace gradido::interaction::serialize;

using QueryHeaderMessage = message<
>;

using ResponseHeaderMessage = message<
>;

using TransactionReceiptMessage = message<
>;

using TransactionGetReceiptQueryMessage = message<
	message_field<"header", 1, QueryHeaderMessage>,
	message_field<"transactionID", 2, HieroTransactionIdMessage>,
	bool_field<"includeDuplicates", 3>, 
	bool_field<"include_child_receipts", 4>
>;

using TransactionGetReceiptResponseMessage = message<
	message_field<"header", 1, ResponseHeaderMessage>,
	message_field<"receipt", 2, TransactionReceiptMessage>,
	message_field<"duplicateTransactionReceipts", 4, TransactionReceiptMessage, repeated>,
	message_field<"child_transaction_receipts", 5, TransactionReceiptMessage, repeated>
>;


namespace client {
	namespace grpc {
		
		Client::Client(std::shared_ptr<::grpc::Channel> channel)
			: mGenericStub(std::make_unique<::grpc::GenericStub>(channel))
		{

		}
		Client::~Client()
		{
		}

		std::shared_ptr<Client> Client::createForTarget(const std::string& targetUrl)
		{
			// load ssl certificate from default folder, overwritten with GRPC_DEFAULT_SSL_ROOTS_FILE_PATH 
			auto sslCredentials = ::grpc::SslCredentials(::grpc::SslCredentialsOptions());
			if (!sslCredentials) {
				throw SSLException("cannot load ssl root certificate, consider setting env GRPC_DEFAULT_SSL_ROOTS_FILE_PATH");
			}
			auto channel = ::grpc::CreateChannel(targetUrl, sslCredentials);
			if (!channel) {
				throw ChannelException("cannot connect to grpc server", targetUrl.data());
			}
			return std::shared_ptr<Client>(new Client(channel));
		}

		void Client::subscribeTopic(
			IMessageObserver* handler,
			const hiero::TopicId& topicId,
			gradido::data::Timestamp consensusStartTime /* = gradido::data::Timestamp() */,
			gradido::data::Timestamp consensusEndTime /* = gradido::data::Timestamp() */,
			uint64_t limit /* = 0 */
		)
		{
			auto rawRequest = serializeConsensusTopicQuery(topicId, consensusStartTime, consensusEndTime, limit);
			auto reactor = new TopicMessageQueryReactor(handler, std::move(rawRequest));
			::grpc::ClientContext context;
			::grpc::StubOptions options;
			// TODO: check intern channel pointer for dangling pointer
			mGenericStub->PrepareBidiStreamingCall(&context, "/ConsensusService/subscribeTopic", options, reactor);
			::grpc::WriteOptions writeOptions;
			reactor->StartWriteLast(reactor->getBuffer(), writeOptions);
		}

		memory::Block Client::serializeConsensusTopicQuery(
			const hiero::TopicId& topicId,
			const gradido::data::Timestamp& consensusStartTime,
			const gradido::data::Timestamp& consensusEndTime,
			uint64_t limit
		) {
			using ConsensusTopicQuery = message<
				message_field<"topicID", 1, HieroTopicIdMessage>,
				message_field<"consensusStartTime", 2, TimestampMessage>,
				message_field<"consensusEndTime", 3, TimestampMessage>,
				uint64_field<"limit", 4>
			>;

			ConsensusTopicQuery message {
				HieroTopicIdMessage { topicId.getShardNum(), topicId.getRealmNum(), topicId.getTopicNum() },
				TimestampMessage { consensusStartTime.getSeconds(), consensusStartTime.getNanos() },
				TimestampMessage { consensusEndTime.getSeconds(), consensusEndTime.getNanos() },
				limit
			};
			constexpr size_t bufferSize = 6 * 8 + 2 * 4 + 6;
			std::array<std::byte, bufferSize> buffer;
			auto bufferEnd = pp::message_coder<ConsensusTopicQuery>::encode(message, buffer);
			if (!bufferEnd.has_value()) {
				throw GradidoNodeInvalidDataException("couldn't serialize to ConsensusTopicQuery");
			}
			auto bufferEndValue = bufferEnd.value();
			auto actualSize = pp::begin_diff(bufferEndValue, buffer);
			if (!actualSize) {
				throw GradidoNodeInvalidDataException("shouldn't be possible");
			}
			return memory::Block(actualSize, (const uint8_t*)buffer.data());
		}
	}
}