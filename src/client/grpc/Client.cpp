#include "Client.h"
#include "Exceptions.h"
#include "SubscriptionReactor.h"
#include "RequestReactor.h"
#include "gradido_blockchain/interaction/serialize/Protopuf.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

using namespace gradido::interaction::serialize;

namespace client {
	namespace grpc {

		using GenericStub = ::grpc::TemplatedGenericStub<::grpc::ByteBuffer, ::grpc::ByteBuffer>;
		
		Client::Client(std::shared_ptr<::grpc::Channel> channel)
			: mChannel(channel)
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
			const hiero::ConsensusTopicQuery& consensusTopicQuery,
			std::shared_ptr<MessageObserver<hiero::ConsensusTopicResponseMessage>> responseListener
		)
		{
			auto rawRequest = serializeConsensusTopicQuery(consensusTopicQuery);
			// will self destroy on connection close call from grpc
			auto reactor = new SubscriptionReactor(responseListener, std::move(rawRequest));
			::grpc::StubOptions options;
			GenericStub genericStub(mChannel);
			genericStub.PrepareBidiStreamingCall(responseListener->getClientContextPtr(), "/ConsensusService/subscribeTopic", options, reactor);
			::grpc::WriteOptions writeOptions;
			reactor->StartWriteLast(reactor->getBuffer(), writeOptions);
		}
		void Client::getTransactionReceipts(
			const hiero::TransactionId& request,
			std::shared_ptr<MessageObserver<hiero::TransactionGetReceiptResponseMessage>> responseListener
		) {
			auto rawRequest = serializeQuery(
				std::make_shared<hiero::TransactionGetReceiptQuery>(
					hiero::QueryHeader(hiero::ResponseType::ANSWER_ONLY),
					request
				)
			);
			::grpc::StubOptions options;
			::grpc::Slice rawRequestSlice(rawRequest.data(), rawRequest.size());
			::grpc::ByteBuffer rawRequestByteBuffer(&rawRequestSlice, 1);
			GenericStub genericStub(mChannel);
			// will self destroy on connection close call from grpc
			auto reactor = new RequestReactor<hiero::TransactionGetReceiptResponseMessage>(responseListener);
			genericStub.PrepareUnaryCall(
				responseListener->getClientContextPtr(),
				"/CryptoService/getTransactionReceipts",
				options,
				&rawRequestByteBuffer,
				reactor->getBuffer(),
				reactor
			);
		}

		memory::Block Client::serializeConsensusTopicQuery(const hiero::ConsensusTopicQuery& consensusTopicQuery) {
			array<std::byte, hiero::ConsensusTopicQuery::maxSize()> buffer;
			auto message = consensusTopicQuery.getMessage();
			auto bufferEnd = message_coder<hiero::ConsensusTopicQueryMessage>::encode(message, buffer);
			if (!bufferEnd.has_value()) {
				throw GradidoNodeInvalidDataException("couldn't serialize to ConsensusTopicQuery");
			}
			auto bufferEndValue = bufferEnd.value();
			auto actualSize = pp::begin_diff(bufferEndValue, buffer);
			if (!actualSize) {
				throw GradidoNodeInvalidDataException("shouldn't be possible");
			}
			LOG_F(2, "consensusTopicQuery serialized size: %llu/%llu", actualSize, hiero::ConsensusTopicQuery::maxSize());
			return memory::Block(actualSize, (const uint8_t*)buffer.data());
		}

		memory::Block Client::serializeQuery(const hiero::Query& query)
		{
			array<std::byte, hiero::Query::maxSize()> buffer;
			auto message = query.getMessage();
			auto bufferEnd = message_coder<hiero::QueryMessage>::encode(message, buffer);
			if (!bufferEnd.has_value()) {
				throw GradidoNodeInvalidDataException("couldn't serialize to Query");
			}
			auto bufferEndValue = bufferEnd.value();
			auto actualSize = pp::begin_diff(bufferEndValue, buffer);
			if (!actualSize) {
				throw GradidoNodeInvalidDataException("shouldn't be possible");
			}
			LOG_F(2, "Query serialized size: %llu/%llu", actualSize, hiero::Query::maxSize());
			return memory::Block(actualSize, (const uint8_t*)buffer.data());
		}
		
	}
}