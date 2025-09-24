#include "TopicMessageQuery.h"
#include "const.h"
#include "MemoryBlock.h"
#include "../../hiero/ConsensusTopicQuery.h"
#include "../../lib/protopuf.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "grpcpp/support/async_stream.h"
#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <chrono>

using namespace magic_enum;

namespace client {
	namespace hiero {

        TopicMessageQuery::TopicMessageQuery(const char* name, ::hiero::ConsensusTopicQuery startQuery)
            : mThread(&TopicMessageQuery::ThreadFunction, this), 
            mThreadName(name), 
            mExitCalled(false), 
            mStartQuery(startQuery),
            mCallStatus(CallStatus::STATUS_CREATE)
        {
        }

        TopicMessageQuery::~TopicMessageQuery()
        {
            LOG_F(INFO, "TopicMessageQuery::~TopicMessageQuery");
            mExitCalled = true;
            mThread.join();
        }

        void TopicMessageQuery::setResponseReader(std::unique_ptr<::grpc::ClientAsyncReaderWriter<::grpc::ByteBuffer, ::grpc::ByteBuffer>>& responseReader) 
        {
            mResponseReader = std::move(responseReader);
        }

        int TopicMessageQuery::ThreadFunction() 
        {
            loguru::set_thread_name(mThreadName.data());
            // copied most of the code from hiero cpp sdk from startSubscription from TopicMessageQuery.cc
            ::hiero::ConsensusTopicQuery query;
            // Declare needed variables.
            ::grpc::ByteBuffer grpcByteBuffer;
            ::hiero::ConsensusTopicResponse response;
            std::chrono::system_clock::duration backoff = ::hiero::DEFAULT_MIN_BACKOFF;
            std::chrono::system_clock::duration maxBackoff = ::hiero::DEFAULT_MAX_BACKOFF;
            ::grpc::Status grpcStatus;
            uint64_t attempt = 0ULL;
            bool complete = false;
            bool ok = false;
            void* tag = nullptr;

            // serialize start query into grpc Byte Buffer
            MemoryBlock memoryBuffer(protopuf::serialize<::hiero::ConsensusTopicQuery, ::hiero::ConsensusTopicQueryMessage>(mStartQuery));
            grpcByteBuffer = memoryBuffer.createGrpcBuffer();

            while (!mExitCalled) {
                // Process based on the completion queue status.
                auto backOffFromNow = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::milliseconds>(backoff);
                auto nextStatus = mCompletionQueue.AsyncNext(&tag, &ok, backOffFromNow);
                LOG_F(2, "next status: %s", enum_name(nextStatus).data());
                switch (nextStatus)
                {
                case ::grpc::CompletionQueue::TIMEOUT:
                {
                    // Backoff if the completion queue timed out.
                    backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
                    LOG_F(2, "new timeout: %s", DataTypeConverter::timespanToString(backoff).data());
                    break;
                }
                case ::grpc::CompletionQueue::GOT_EVENT:
                {
                    // Decrease the backoff time.
                    backoff = (backoff / 2 < ::hiero::DEFAULT_MIN_BACKOFF) ? ::hiero::DEFAULT_MIN_BACKOFF : backoff / 2;

                    // Process based on the call status.
                    switch (static_cast<CallStatus>(*reinterpret_cast<long*>(tag)))
                    {
                    case CallStatus::STATUS_CREATE:
                    {
                        if (ok)
                        {
                            ::grpc::WriteOptions writeOptions;
                            mCallStatus = CallStatus::STATUS_WRITE;
                            mResponseReader->WriteLast(grpcByteBuffer, writeOptions, &mCallStatus);
                        }
                        break;
                    }
                    case CallStatus::STATUS_WRITE:
                    {
                        if (ok)
                        {
                            mCallStatus = CallStatus::STATUS_PROCESSING;
                            mResponseReader->Read(&grpcByteBuffer, &mCallStatus);
                        }
                        break;
                    }
                    case CallStatus::STATUS_PROCESSING:
                    {
                        // If the response should be processed, process it.
                        if (ok)
                        {
                            // Read the response.
                            auto block = MemoryBlock(grpcByteBuffer);
                            LOG_F(INFO, "response: echo \"%s\" | xxd -r -p | protoscope", block.get()->convertToHex().data());
                            response = protopuf::deserialize<::hiero::ConsensusTopicResponse, ::hiero::ConsensusTopicResponseMessage>(*block.get());

                            // Adjust the query timestamp and limit, in case a retry is triggered.
                            auto consensusTimestamp = response.getConsensusTimestamp();
                            if (!consensusTimestamp.empty())
                            {
                                // Add one of the smallest denomination of time
                                query.setConsensusStartTime({
                                    consensusTimestamp.getSeconds(), consensusTimestamp.getNanos() + 1
                                });
                            }

                            if (query.getLimit() > 0ULL)
                            {
                                query.setLimit(query.getLimit() - 1ULL);
                            }

                            // Process the received message.
                            const auto& chunkInfo = response.getChunkInfo();
                            if (chunkInfo.empty() || chunkInfo.getTotal() == 1)
                            {
                                onMessageArrived(std::move(response));
                            }
                            else
                            {
                                LOG_F(FATAL, "Chunked Messages not implemented yet");
                                /*
                                const TransactionId transactionId =
                                    TransactionId::fromProtobuf(response.chunkinfo().initialtransactionid());
                                pendingMessages[transactionId].push_back(response);

                                if (pendingMessages[transactionId].size() == response.chunkinfo().total())
                                {
                                    onNext(TopicMessage::ofMany(pendingMessages[transactionId]));
                                }
                                */
                            }
                            mResponseReader->Read(&grpcByteBuffer, &mCallStatus);
                        }

                        // If the response shouldn't be processed (due to completion or error), finish the RPC.
                        else
                        {
                            mCallStatus = CallStatus::STATUS_FINISH;
                            mResponseReader->Finish(&grpcStatus, &mCallStatus);
                        }

                        break;
                    }
                    case CallStatus::STATUS_FINISH:
                    {
                        if (grpcStatus.ok())
                        {
                            // RPC completed successfully.
                            // completionHandler();
                            LOG_F(INFO, "RPC subscription complete!");

                            // Shutdown the completion queue.
                            mCompletionQueue.Shutdown();

                            // Mark the RPC as complete.
                            complete = true;
                            break;
                        }
                        else
                        {
                            // An error occurred. Whether retrying or not, cancel the call and close the queue.
                            mClientContext.TryCancel();
                            mCompletionQueue.Shutdown();

                            if (attempt >= ::hiero::DEFAULT_MAX_ATTEMPTS || !shouldRetry(grpcStatus))
                            {
                                // This RPC call shouldn't be retried, handle the error and mark as complete to exit.
                                //errorHandler(grpcStatus);
                                LOG_F(ERROR, "Subscription error: %s", grpcStatus.error_message().data());
                                complete = true;
                            }
                        }

                        break;
                    }
                    default:
                    {
                        // Unrecognized call status, do nothing for now (not sure if this is correct).
                        break;
                    }
                    }

                    break;
                }
                case ::grpc::CompletionQueue::SHUTDOWN:
                {
                    // Getting here means the RPC is reached completion or encountered an un-retriable error, and the completion
                    // queue has been shut down. End the subscription.
                    if (complete)
                    {
                        // Give a second for the queue to finish its processing.
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        return 0;
                    }

                    // If the completion queue has been shut down and the RPC hasn't completed, that means the RPC needs to be
                    // retried. Increase the backoff for the retry.
                    backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
                    std::this_thread::sleep_for(backoff);
                    ++attempt;

                    // Resend the query to a different node with a different completion queue and client context.
                    // contexts.push_back(std::make_unique<grpc::ClientContext>());
                    // queues.push_back(std::make_unique<grpc::CompletionQueue>());

                    // Reset the call status and send the query.
                    /*
                    *callStatus = CallStatus::STATUS_CREATE;
                    reader = getConnectedMirrorNode(network)->getConsensusServiceStub()->AsyncsubscribeTopic(
                        contexts.back().get(), query, queues.back().get(), callStatus.get());
                        */
                    onConnectionClosed();
                    break;
                }
                default:
                {
                    // Not sure what to do here, just fail out.
                    std::cout << "Unknown gRPC completion queue event, failing.." << std::endl;
                    return -1;
                }
                }
            }
            return 0;
        }

        bool TopicMessageQuery::shouldRetry(::grpc::Status status)
        {
            return (status.error_code() == ::grpc::StatusCode::NOT_FOUND) ||
                (status.error_code() == ::grpc::StatusCode::RESOURCE_EXHAUSTED) ||
                (status.error_code() == ::grpc::StatusCode::UNAVAILABLE) ||
                (status.error_code() == ::grpc::StatusCode::INTERNAL);
        }
	}
}
