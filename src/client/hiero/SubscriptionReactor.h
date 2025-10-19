#ifndef __GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_REACTOR_H
#define __GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_REACTOR_H

#include <string>
#include <bit>
#include "protopuf/message.h"

#include "../Exceptions.h"
#include "MessageObserver.h"
#include "../../lib/FuzzyTimer.h"
#include "../../SingletonManager/CacheManager.h"

#include "gradido_blockchain/lib/Profiler.h"

#include <grpcpp/support/client_callback.h>
#include <grpcpp/impl/status.h>
#include <grpcpp/support/byte_buffer.h>

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

/*
* class TimerCallback
{
public:
	virtual TimerReturn callFromTimer() = 0;
	virtual const char* getResourceType() const { return "TimerCallback"; };
};
*/

namespace client {
	namespace hiero {
		template<class T>
		class SubscriptionReactor : public grpc::ClientBidiReactor<grpc::ByteBuffer, grpc::ByteBuffer>, public TimerCallback
		{
		public:
			SubscriptionReactor(std::shared_ptr<MessageObserver<T>> messageObserver, const MemoryBlock& rawRequest)
				: mMessageObserver(messageObserver)
			{
				if (!messageObserver) {
					throw GradidoNullPointerException(
						"empty messageObserver", 
						"MessageObserver", 
						"client::grpc::SubscriptionReactor"
					);
				}
				mBuffer = rawRequest.createGrpcBuffer();
			}
			virtual ~SubscriptionReactor() {
				LOG_F(WARNING, "~SubscriptionReactor()");
				// CacheManager::getInstance()->getFuzzyTimer()->removeTimer(std::to_string((long long)this));
			}
			const char* getResourceType() const override { return "SubscriptionReactor"; };
			TimerReturn callFromTimer() override {
				StartRead(&mBuffer);
				return TimerReturn::REMOVE_ME;
			}
			/// Notifies the application that a StartWrite or StartWriteLast operation
			/// completed.
			///
			/// \param[in] ok Was it successful? If false, no new read/write operation
			///               will succeed.
			virtual void OnWriteDone(bool ok) {
				if (!ok) {
					LOG_F(ERROR, "write operation failed, need to restart subscription");
					mMessageObserver->onConnectionClosed();
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
				if (!ok) {
					LOG_F(WARNING, "read done with ok = false");
					// LOG_F(ERROR, "read operation failed, need to restart subscription");
					// mMessageObserver->onConnectionClosed();
					// CacheManager::getInstance()->getFuzzyTimer()->addTimer(std::to_string((long long)this), this, std::chrono::milliseconds(250), 1);
					return;
				}
				Profiler timeUsed;
				auto block = MemoryBlock(mBuffer);
				auto result = pp::message_coder<T>::decode(block.get()->span());
				if (!result.has_value()) {
					LOG_F(ERROR, "couldn't be deserialized: echo \"%s\" | xxd -r -p | protoscope", block.get()->convertToHex().data());
				}
				else {
					const auto& [message, bufferEnd2] = *result;
					LOG_F(1, "timeUsed for deserialize: %s", timeUsed.string().data());
					mMessageObserver->onMessageArrived(message);
					LOG_F(1, "OnReadDone");
				}
				
				// after on read was successful we await the next
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
			virtual void OnDone(const grpc::Status& s)
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
				mMessageObserver->onConnectionClosed();
				delete this;
			}
			grpc::ByteBuffer* getBuffer() { return &mBuffer; }
		protected:
			std::shared_ptr<MessageObserver<T>> mMessageObserver;
			grpc::ByteBuffer mBuffer;
		};

	}
}

#endif //__GRADIDO_NODE_CLIENT_HIERO_TOPIC_MESSAGE_QUERY_REACTOR_H