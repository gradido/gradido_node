#ifndef __GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_H
#define __GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_H

#include "../../hiero/ConsensusTopicResponse.h"
#include "../../hiero/Query.h"
#include "MemoryBlock.h"

#include "../../task/Thread.h"

#include <grpcpp/completion_queue.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

namespace client {
	namespace hiero {

		// Enum to track the status of the gRPC call.
		enum class CallStatus : long
		{
			STATUS_CREATE = 0,
			STATUS_WRITE = 1,
			STATUS_PROCESSING = 2,
			STATUS_FINISH = 3
		};

		enum class ConnectionClosedReason 
		{
			Subscription_Ended,
			Error,
			Exception,
			Reconnect,
			Deconstruct,
			Timeout // it seems that grpc/hiero topic subscribe is unstable around ~ 30 minutes without messages so better close connection than and create new one
		};

		class TopicMessageQuery
		{
		public:
			TopicMessageQuery(const char* name, ::hiero::ConsensusTopicQuery startQuery);
			virtual ~TopicMessageQuery();

			// no copies allowed
			TopicMessageQuery(const TopicMessageQuery&) = delete;
			TopicMessageQuery& operator=(const TopicMessageQuery&) = delete;

			// no moves allowed
			TopicMessageQuery(TopicMessageQuery&&) = delete;
			TopicMessageQuery& operator=(TopicMessageQuery&&) = delete;

			grpc::CompletionQueue* getCompletionQueuePtr() { return mCompletionQueues.back().get(); }
			grpc::ClientContext* getClientContextPtr() { return mClientContexts.back().get(); }

			void setResponseReader(std::unique_ptr<grpc::ClientAsyncReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>>& responseReader);

			virtual void onMessageArrived(::hiero::ConsensusTopicResponse&& response) = 0;

			// will be called from grpc client if connection was closed
			// so no more messageArrived calls
			virtual void onConnectionClosed(ConnectionClosedReason reason) noexcept = 0;

			// called inside loop if connection was closed, default implementation copied from hiero cpp sdk
			virtual bool shouldRetry(grpc::Status status);

			CallStatus* getCallStatusPtr() { return &mCallStatus; }

		protected:
			int ThreadFunction();
			std::thread mThread;
			std::string mThreadName;
			std::atomic<bool> mExitCalled;

			::hiero::ConsensusTopicQuery mStartQuery;

			std::vector<std::unique_ptr<grpc::CompletionQueue>> mCompletionQueues;
			std::vector<std::unique_ptr<grpc::ClientContext>> mClientContexts;
			CallStatus mCallStatus;
			std::unique_ptr<grpc::ClientAsyncReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>> mResponseReader;
		};
	}
}
#endif //__GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_H