#ifndef __GRADIDO_NODE_CLIENT_GRPC_TOPIC_MESSAGE_QUERY_REACTOR_H
#define __GRADIDO_NODE_CLIENT_GRPC_TOPIC_MESSAGE_QUERY_REACTOR_H

#include "grpcpp/support/client_callback.h"
#include "grpcpp/impl/status.h"
#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"
#include "../Exceptions.h"
#include "IMessageObserver.h"

#include <grpcpp/support/byte_buffer.h>
#include <atomic>

namespace client {
	namespace grpc {
		class TopicMessageQueryReactor : public ::grpc::ClientBidiReactor<::grpc::ByteBuffer, ::grpc::ByteBuffer>
		{
		public:
			TopicMessageQueryReactor(IMessageObserver* callback, memory::Block&& rawRequest)
				: mCallback(callback), mIsFinished(false)
			{
				if (!callback) {
					throw GradidoNullPointerException(
						"empty callback", 
						"IMessageObserver", 
						"client::grpc::TopicMessageQueryReactor"
					);
				}
				::grpc::Slice slice(rawRequest.data(), rawRequest.size());
				mBuffer = ::grpc::ByteBuffer(&slice, 1);

			}
			/// Notifies the application that a StartWrite or StartWriteLast operation
			/// completed.
			///
			/// \param[in] ok Was it successful? If false, no new read/write operation
			///               will succeed.
			virtual void OnWriteDone(bool ok) {
				if (mIsFinished) {
					LOG_F(WARNING, "OnWriteDone called after mIsFinished was already set to true");
					return;
				}
				if (!ok) {
					LOG_F(ERROR, "write operation failed, need to restart subscription");
					mCallback->messagesStopped();
					mIsFinished = true;
				}
				LOG_F(1, "OnWriteDone");
				// after topic subscription request was transfered, we read listen to the result
				StartRead(&mBuffer);
			}
			/// Notifies the application that a StartRead operation completed.
			///
			/// \param[in] ok Was it successful? If false, no new read/write operation
			///               will succeed.
			virtual void OnReadDone(bool ok) {
				if (mIsFinished) {
					LOG_F(WARNING, "OnReadDone called after mIsFinished was already set to true");
					return;
				}
				if (!ok) {
					LOG_F(ERROR, "read operation failed, need to restart subscription");
					mCallback->messagesStopped();
					mIsFinished = true;
				}
				LOG_F(1, "OnReadDone");
				::grpc::Slice slice;
				mBuffer.TrySingleSlice(&slice);
				mCallback->messageArrived(memory::Block(slice.size(), slice.begin()));
				// after on read was successfull we await the next
				StartRead(&mBuffer);
			}

			/// Notifies the application that all operations associated with this RPC
			/// have completed and all Holds have been removed. OnDone provides the RPC
			/// status outcome for both successful and failed RPCs and will be called in
			/// all cases. If it is not called, it indicates an application-level problem
			/// (like failure to remove a hold).
			///
			/// OnDone is called exactly once, and not concurrently with any (other)
			/// reaction. (Holds may be needed (see above) to prevent OnDone from being
			/// called concurrently with calls to the reactor from outside of reactions.)
			///
			/// \param[in] s The status outcome of this RPC
			virtual void OnDone(const ::grpc::Status& s)
			{
				if (!s.ok()) {
					/// Return the instance's error code.
					// StatusCode error_code() const { return code_; }
					/// Return the instance's error message.
					// std::string error_message() const { return error_message_; }
					/// Return the (binary) error details.
					// Usually it contains a serialized google.rpc.Status proto.
					// std::string error_details() const { return binary_error_details_; }
					LOG_F(ERROR, "onDone called from grpc with status: %s, error: %s",
						magic_enum::enum_name<::grpc::StatusCode>(s.error_code()).data(),
						s.error_message().data()
					);
				}
				LOG_F(1, "OnDone");
				mCallback->messagesStopped();
				mIsFinished = true;
				delete this;
			}
			bool isFinished() const { return mIsFinished; }
			::grpc::ByteBuffer* getBuffer() { return &mBuffer; }
		protected:
			IMessageObserver* mCallback;
			std::atomic<bool> mIsFinished;
			::grpc::ByteBuffer mBuffer;
		};

	}
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_TOPIC_MESSAGE_QUERY_REACTOR_H