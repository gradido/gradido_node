#ifndef __GRADIDO_NODE_CLIENT_GRPC_REQUEST_REACTOR_H
#define __GRADIDO_NODE_CLIENT_GRPC_REQUEST_REACTOR_H

#include <string>
#include <bit>
#include "protopuf/message.h"

#include "../Exceptions.h"
#include "MessageObserver.h"

#include "gradido_blockchain/lib/Profiler.h"

#include <grpcpp/support/client_callback.h>
#include <grpcpp/impl/status.h>
#include <grpcpp/support/byte_buffer.h>

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

namespace client {
	namespace grpc {
        template<class T>
        class RequestReactor : public ::grpc::ClientUnaryReactor {
        public:
            RequestReactor(std::shared_ptr<MessageObserver<T>> messageObserver)
                : mMessageObserver(messageObserver)
            {
                if (!messageObserver) {
                    throw GradidoNullPointerException(
                        "empty messageObserver",
                        "MessageObserver",
                        "client::grpc::RequestReactor"
                    );
                }
            }

            void OnDone(const ::grpc::Status& s) {
                if (!s.ok()) {
                    LOG_F(ERROR, "onDone called from grpc with status: %s, error: %s, details: %s",
                        magic_enum::enum_name<::grpc::StatusCode>(s.error_code()).data(),
                        s.error_message().data(),
                        s.error_details().data()
                    );
                }
                else {
                    Profiler timeUsed;
                    ::grpc::Slice slice;
                    mBuffer.TrySingleSlice(&slice);
                    auto data = pp::bytes(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(slice.begin())), slice.size());
                    auto result = pp::message_coder<T>::decode(data);
                    if (!result.has_value()) {
                        LOG_F(ERROR, "on_completion result couldn't be deserialized");
                    }
                    else {
                        const auto& [message, bufferEnd2] = *result;
                        LOG_F(1, "timeUsed for deserialize: %s", timeUsed.string().data());
                        try {
                            mMessageObserver->onMessageArrived(message);
                        }
                        catch (GradidoBlockchainException& ex) {
                            LOG_F(ERROR, ex.getFullString().data());
                        }
                        catch (std::exception& ex) {
                            LOG_F(ERROR, ex.what());
                        }
                    }
                }
                mMessageObserver->onConnectionClosed();
                delete this;
            }
            ::grpc::ByteBuffer* getBuffer() { return &mBuffer; }
        protected:
            std::shared_ptr<MessageObserver<T>> mMessageObserver;
            ::grpc::ByteBuffer mBuffer;
        };
    }
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_REQUEST_REACTOR_H