#include "CertificateVerifier.h"
#include "ConsensusClient.h"
#include "Exceptions.h"
#include "MemoryBlock.h"
#include "RequestReactor.h"

#include "../../hiero/ConsensusGetTopicInfoQuery.h"
#include "../../hiero/TransactionGetReceiptQuery.h"
#include "../../hiero/Query.h"
#include "../../lib/protopuf.h"

#include "grpcpp/channel.h"
#include "grpcpp/generic/generic_stub.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/support/channel_arguments.h"
#include "grpcpp/support/stub_options.h"

using std::shared_ptr, std::make_shared, std::string;

namespace client {
	namespace hiero {

		template<class ResponseMessage>
		void unaryCall(
			const ::hiero::Query& query, 
			shared_ptr<MessageObserver<ResponseMessage>> responseListener, 
			const char* method, 
			shared_ptr<grpc::Channel> channel
		) {
			grpc::StubOptions options;
			grpc::TemplatedGenericStub<grpc::ByteBuffer, grpc::ByteBuffer> consensusStub(channel);
			auto rawRequest = MemoryBlock(protopuf::serialize<::hiero::Query, ::hiero::QueryMessage>(query));
			LOG_F(INFO, "echo \"%s\" | xxd -r -p | protoscope", rawRequest.get()->convertToHex().data());
			auto grpcByteBuffer = rawRequest.createGrpcBuffer();

			// will self destroy on connection close call from grpc
			auto reactor = new RequestReactor<ResponseMessage>(responseListener);
			consensusStub.PrepareUnaryCall(
				responseListener->getClientContextPtr(),
				method,
				options,
				&grpcByteBuffer,
				reactor->getBuffer(),
				reactor
			);
			reactor->StartCall();
		}

		ConsensusClient::ConsensusClient(shared_ptr<grpc::Channel> channel)
			: mChannel(channel)
		{
		}

		ConsensusClient::~ConsensusClient()
		{

		}

		std::shared_ptr<ConsensusClient> ConsensusClient::createForTarget(
			const string& targetUrl,
			bool isTransportSecurity,
			memory::ConstBlockPtr certificateHash
		) {
			std::shared_ptr<grpc::ChannelCredentials> credentials;
			if (isTransportSecurity) {
				credentials = getTlsChannelCredentials(certificateHash);
			}
			else {
				credentials = grpc::InsecureChannelCredentials();
			}
			auto mirrorCredentials = grpc::experimental::TlsCredentials(grpc::experimental::TlsChannelCredentialsOptions());

			grpc::ChannelArguments channelArguments;
			channelArguments.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
			channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
			channelArguments.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

			auto channel = grpc::CreateCustomChannel(targetUrl, credentials, channelArguments);
			if (!channel) {
				throw ChannelException("cannot connect to grpc server", targetUrl.data());
			}
			return shared_ptr<ConsensusClient>(new ConsensusClient(channel));
		}

		void ConsensusClient::getTransactionReceipts(
			const ::hiero::TransactionId& transactionId,
			std::shared_ptr<MessageObserver<::hiero::TransactionGetReceiptResponseMessage>> responseListener
		) {
			::hiero::Query query({ ::hiero::QueryHeader(::hiero::ResponseType::ANSWER_ONLY), transactionId });
			unaryCall<::hiero::TransactionGetReceiptResponseMessage>(query, responseListener, "/proto.CryptoService/getTransactionReceipts", mChannel);
		}

		void ConsensusClient::getTopicInfo(
			const ::hiero::TopicId& topicId,
			shared_ptr<MessageObserver<::hiero::ConsensusGetTopicInfoResponseMessage>> responseListener
		) {
			::hiero::Query query({ ::hiero::QueryHeader(::hiero::ResponseType::ANSWER_ONLY), topicId });
			unaryCall<::hiero::ConsensusGetTopicInfoResponseMessage>(query, responseListener, "/proto.ConsensusService/getTopicInfo", mChannel);
		}

		shared_ptr<grpc::ChannelCredentials> ConsensusClient::getTlsChannelCredentials(memory::ConstBlockPtr certificateHash)
		{
			// code copied from hedera c++ sdk
			grpc::experimental::TlsChannelCredentialsOptions tlsChannelCredentialsOptions;

			// Custom verification using the node's certificate chain hash
			tlsChannelCredentialsOptions.set_verify_server_certs(false);
			tlsChannelCredentialsOptions.set_check_call_host(false);

			tlsChannelCredentialsOptions.set_certificate_verifier(
				grpc::experimental::ExternalCertificateVerifier::Create<CertificateVerifier>(certificateHash)
			);

			// Feed in the root CA's file manually for Windows (this is a bug in the gRPC implementation and says here
			// https://deploy-preview-763--grpc-io.netlify.app/docs/guides/auth/#using-client-side-ssltls that this needs to be
			// specified manually).
#ifdef _WIN32
			tlsChannelCredentialsOptions.set_certificate_provider(
				make_shared<grpc::experimental::FileWatcherCertificateProvider>(CACERT_PEM_PATH, -1));
			tlsChannelCredentialsOptions.watch_root_certs();
#endif
			auto tlsCredentials = grpc::experimental::TlsCredentials(tlsChannelCredentialsOptions);
			if (!tlsCredentials) {
				throw SSLException("cannot load ssl root certificate, consider setting env GRPC_DEFAULT_SSL_ROOTS_FILE_PATH");
			}
			return tlsCredentials;
		}
	}
}