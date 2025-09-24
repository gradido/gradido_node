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
		enum class CallStatus
		{
			STATUS_CREATE = 0,
			STATUS_WRITE = 1,
			STATUS_PROCESSING = 2,
			STATUS_FINISH = 3
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

			grpc::CompletionQueue* getCompletionQueuePtr() { return &mCompletionQueue; }
			grpc::ClientContext* getClientContextPtr() { return &mClientContext; }

			void setResponseReader(std::unique_ptr<grpc::ClientAsyncReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>>& responseReader);

			virtual void onMessageArrived(::hiero::ConsensusTopicResponse&& response) = 0;

			// will be called from grpc client if connection was closed
			// so no more messageArrived calls
			virtual void onConnectionClosed() = 0;

			// called inside loop if connection was closed, default implementation copied from hiero cpp sdk
			virtual bool shouldRetry(grpc::Status status);

			CallStatus* getCallStatusPtr() { return &mCallStatus; }

		protected:
			int ThreadFunction();
			std::thread mThread;
			std::string mThreadName;
			std::atomic<bool> mExitCalled;

			::hiero::ConsensusTopicQuery mStartQuery;

			grpc::CompletionQueue mCompletionQueue;
			grpc::ClientContext mClientContext;
			CallStatus mCallStatus;
			std::unique_ptr<grpc::ClientAsyncReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>> mResponseReader;
		};
	}
}
#endif //__GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_H