#include "../../ServerGlobals.h"
#include "../hiero/MirrorClient.h"

#include "CertificateVerifier.h"
#include "Client.h"
#include "Exceptions.h"
#include "MemoryBlock.h"
#include "RequestReactor.h"
#include "SubscriptionReactor.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>


#ifdef _WIN32
#include <fstream>
#include <sstream>
#endif //_WIN32


namespace client {
	namespace grpc {

		using GenericStub = ::grpc::TemplatedGenericStub<::grpc::ByteBuffer, ::grpc::ByteBuffer>;
		
		Client::Client(std::shared_ptr<::grpc::Channel> channel, std::shared_ptr<::grpc::Channel> mirrorChannel)
			: mChannel(channel), mMirrorChannel(mirrorChannel)
		{
			
		}

		std::shared_ptr<Client> Client::createForTarget(
			const std::string& targetUrl,
			bool isTransportSecurity, 
			memory::ConstBlockPtr certificateHash
		) {			
			std::shared_ptr<::grpc::ChannelCredentials> credentials;
			if (isTransportSecurity) {
				credentials = getTlsChannelCredentials(certificateHash);
			}
			else {
				credentials = ::grpc::InsecureChannelCredentials();
			}
			auto mirrorCredentials = ::grpc::experimental::TlsCredentials(::grpc::experimental::TlsChannelCredentialsOptions());

			::grpc::ChannelArguments channelArguments;
			channelArguments.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
			channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
			channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 30000);
			channelArguments.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
			channelArguments.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);

			/*
			if (const std::string authority = getAuthority(); !authority.empty())
			{
				channelArguments.SetString(GRPC_ARG_DEFAULT_AUTHORITY, authority);
			}
			*/
			auto channel = ::grpc::CreateCustomChannel(targetUrl, credentials, channelArguments);
			if (!channel) {
				throw ChannelException("cannot connect to grpc server", targetUrl.data());
			}
			auto mirrorChannel = ::grpc::CreateCustomChannel(ServerGlobals::g_HieroMirrorNode->getHost(), mirrorCredentials, channelArguments);
			return std::shared_ptr<Client>(new Client(channel, mirrorChannel));
		}

		void Client::subscribeTopic(
			const ::hiero::ConsensusTopicQuery& consensusTopicQuery,
			std::shared_ptr<MessageObserver<::hiero::ConsensusTopicResponseMessage>> responseListener
			// SubscriptionThread* responseListener
		)
		{
			if (!responseListener) {
				throw GradidoNullPointerException("missing response listener", "SubscriptionThread", __FUNCTION__);
			}
			MemoryBlock rawRequest = serializeConsensusTopicQuery(consensusTopicQuery);
			// will self destroy on connection close call from grpc
			auto reactor = new SubscriptionReactor(responseListener, rawRequest);
			
			::grpc::StubOptions options;
			GenericStub genericStub(mMirrorChannel);
			genericStub.PrepareBidiStreamingCall(
				responseListener->getClientContextPtr(), 
				"/com.hedera.mirror.api.proto.ConsensusService/subscribeTopic", 
				options, 
				reactor
			);
			::grpc::WriteOptions writeOptions;
			reactor->StartWriteLast(reactor->getBuffer(), writeOptions);
			reactor->StartCall();
			/*
			auto readerWriter = std::move(genericStub.PrepareCall(
				responseListener->getClientContextPtr(),
				"/com.hedera.mirror.api.proto.ConsensusService/subscribeTopic",
				responseListener->getCompletionQueuePtr()
			));
			responseListener->setResponseReader(readerWriter);
			::grpc::WriteOptions writeOptions;
			readerWriter->WriteLast(rawRequest.createGrpcBuffer(), writeOptions, nullptr);
			
			// resume thread
			responseListener->condSignal();			

			readerWriter->StartCall(nullptr);
			*/
		}
		void Client::getTransactionReceipts(
			const ::hiero::TransactionId& transactionId,
			std::shared_ptr<MessageObserver<::hiero::TransactionGetReceiptResponseMessage>> responseListener
		) {
			auto rawRequest = serializeQuery(
				::hiero::TransactionGetReceiptQuery(
					::hiero::QueryHeader(::hiero::ResponseType::ANSWER_ONLY),
					transactionId
				)
			);
			::grpc::StubOptions options;
			::grpc::Slice rawRequestSlice(rawRequest.data(), rawRequest.size());
			::grpc::ByteBuffer rawRequestByteBuffer(&rawRequestSlice, 1);
			GenericStub genericStub(mChannel);
			// will self destroy on connection close call from grpc
			auto reactor = new RequestReactor<::hiero::TransactionGetReceiptResponseMessage>(responseListener);
			genericStub.PrepareUnaryCall(
				responseListener->getClientContextPtr(),
				"/proto.CryptoService/getTransactionReceipts",
				options,
				&rawRequestByteBuffer,
				reactor->getBuffer(),
				reactor
			);
			reactor->StartCall();
		}

		void Client::getTopicInfo(
			const ::hiero::TopicId& topicId,
			std::shared_ptr<MessageObserver<::hiero::ConsensusGetTopicInfoResponseMessage>> responseListener
		) {
			auto rawRequest = serializeQuery(
				::hiero::ConsensusGetTopicInfoQuery(
					::hiero::QueryHeader(::hiero::ResponseType::ANSWER_ONLY),
					topicId
				)
			);
		
			::grpc::StubOptions options;
			::grpc::Slice rawRequestSlice(rawRequest.data(), rawRequest.size());
			::grpc::ByteBuffer rawRequestByteBuffer(&rawRequestSlice, 1);
			GenericStub genericStub(mChannel);
			// will self destroy on connection close call from grpc
			auto reactor = new RequestReactor<::hiero::ConsensusGetTopicInfoResponseMessage>(responseListener);
			genericStub.PrepareUnaryCall(
				responseListener->getClientContextPtr(),
				"/proto.ConsensusService/getTopicInfo",
				options,
				&rawRequestByteBuffer,
				reactor->getBuffer(),
				reactor
			);
			reactor->StartCall();
		}

		std::shared_ptr<::grpc::ChannelCredentials> Client::getTlsChannelCredentials(memory::ConstBlockPtr certificateHash)
		{
			// code copied from hedera c++ sdk
			::grpc::experimental::TlsChannelCredentialsOptions tlsChannelCredentialsOptions;

			// Custom verification using the node's certificate chain hash
			tlsChannelCredentialsOptions.set_verify_server_certs(false);
			tlsChannelCredentialsOptions.set_check_call_host(false);

			tlsChannelCredentialsOptions.set_certificate_verifier(
				::grpc::experimental::ExternalCertificateVerifier::Create<CertificateVerifier>(certificateHash)
			);

			// Feed in the root CA's file manually for Windows (this is a bug in the gRPC implementation and says here
			// https://deploy-preview-763--grpc-io.netlify.app/docs/guides/auth/#using-client-side-ssltls that this needs to be
			// specified manually).
#ifdef _WIN32
			tlsChannelCredentialsOptions.set_certificate_provider(
				std::make_shared<::grpc::experimental::FileWatcherCertificateProvider>(CACERT_PEM_PATH, -1));
			tlsChannelCredentialsOptions.watch_root_certs();
#endif
			auto tlsCredentials = ::grpc::experimental::TlsCredentials(tlsChannelCredentialsOptions);
			if (!tlsCredentials) {
				throw SSLException("cannot load ssl root certificate, consider setting env GRPC_DEFAULT_SSL_ROOTS_FILE_PATH");
			}
			return tlsCredentials;
		}

		memory::Block Client::serializeConsensusTopicQuery(const ::hiero::ConsensusTopicQuery& consensusTopicQuery) {
			array<std::byte, ::hiero::ConsensusTopicQuery::maxSize()> buffer;
			auto message = consensusTopicQuery.getMessage();
			auto bufferEnd = message_coder<::hiero::ConsensusTopicQueryMessage>::encode(message, buffer);
			if (!bufferEnd.has_value()) {
				throw GradidoNodeInvalidDataException("couldn't serialize to ConsensusTopicQuery");
			}
			auto bufferEndValue = bufferEnd.value();
			auto actualSize = pp::begin_diff(bufferEndValue, buffer);
			if (!actualSize) {
				throw GradidoNodeInvalidDataException("shouldn't be possible");
			}
			LOG_F(2, "consensusTopicQuery serialized size: %llu/%llu", actualSize, ::hiero::ConsensusTopicQuery::maxSize());
			return memory::Block(actualSize, (const uint8_t*)buffer.data());
		}

		memory::Block Client::serializeQuery(const ::hiero::Query& query)
		{
			array<std::byte, ::hiero::Query::maxSize()> buffer;
			auto message = query.getMessage();
			auto bufferEnd = message_coder<::hiero::QueryMessage>::encode(message, buffer);
			if (!bufferEnd.has_value()) {
				throw GradidoNodeInvalidDataException("couldn't serialize to Query");
			}
			auto bufferEndValue = bufferEnd.value();
			auto actualSize = pp::begin_diff(bufferEndValue, buffer);
			if (!actualSize) {
				throw GradidoNodeInvalidDataException("shouldn't be possible");
			}
			LOG_F(2, "Query serialized size: %llu/%llu", actualSize, ::hiero::Query::maxSize());
			return memory::Block(actualSize, (const uint8_t*)buffer.data());
		}
		
	}
}