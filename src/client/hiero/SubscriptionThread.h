#ifndef __GRADIDO_NODE_CLIENT_HIERO_SUBSCRIPTION_THREAD_H
#define __GRADIDO_NODE_CLIENT_HIERO_SUBSCRIPTION_THREAD_H

#include "../grpc/MemoryBlock.h"

#include "../../task/Thread.h"

#include <grpcpp/completion_queue.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

namespace client {
	namespace hiero {

		class SubscriptionThread : public task::Thread
		{
		public:
			SubscriptionThread(const char* name): task::Thread(name) {}
			virtual ~SubscriptionThread() {}

			::grpc::CompletionQueue* getCompletionQueuePtr() { return &mCompletionQueue; }
			::grpc::ClientContext* getClientContextPtr() { return &mClientContext; }

			// inline void setResponseReader(std::unique_ptr<::grpc::ClientAsyncReaderWriter<::grpc::ByteBuffer, ::grpc::ByteBuffer>>& responseReader);

			virtual void onMessageArrived(const grpc::MemoryBlock& rawBuffer) = 0;

			// will be called from grpc client if connection was closed
			// so no more messageArrived calls
			virtual void onConnectionClosed() = 0;

		protected:
      
			int ThreadFunction() override;

			::grpc::CompletionQueue mCompletionQueue;
			::grpc::ClientContext mClientContext;
			//std::unique_ptr<::grpc::ClientAsyncReaderWriter<::grpc::ByteBuffer, ::grpc::ByteBuffer>> mResponseReader;
		};

		/*void SubscriptionThread::setResponseReader(std::unique_ptr<::grpc::ClientAsyncReaderWriter<::grpc::ByteBuffer, ::grpc::ByteBuffer>>& responseReader) {
			mResponseReader = std::move(responseReader);
		}*/
	}
}
#endif //__GRADIDO_NODE_CLIENT_HIERO_SUBSCRIPTION_THREAD_H