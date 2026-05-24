#include "TopicMessageQuery.h"
#include "const.h"
#include "MemoryBlock.h"
#include "MirrorClient.h"
#include "../../hiero/ConsensusTopicQuery.h"
#include "../../lib/protopuf.h"
#include "../../ServerGlobals.h"

#include "gradido_blockchain/Application.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "grpcpp/support/async_stream.h"
#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <chrono>

using std::chrono::duration_cast, std::chrono::milliseconds, std::chrono::seconds, std::chrono::system_clock;
using DataTypeConverter::timespanToString;
using namespace magic_enum;
using std::make_unique;
using std::this_thread::sleep_for;

namespace client {
	namespace hiero {

        TopicMessageQuery::TopicMessageQuery(const char* name, ::hiero::ConsensusTopicQuery startQuery)
            : mThreadName(name),
            mExitCalled(false),
            mStartQuery(startQuery),
            mCallStatus(CallStatus::STATUS_CREATE)
        {
            mCompletionQueues.push_back(make_unique<grpc::CompletionQueue>());
            mClientContexts.push_back(make_unique<grpc::ClientContext>());
            mThread = std::thread(&TopicMessageQuery::ThreadFunction, this);
        }

        TopicMessageQuery::~TopicMessageQuery()
        {
            LOG_F(2, "TopicMessageQuery::~TopicMessageQuery");
            mExitCalled = true;
            mThread.join();
        }

        void TopicMessageQuery::setResponseReader(std::unique_ptr<::grpc::ClientAsyncReaderWriter<::grpc::ByteBuffer, ::grpc::ByteBuffer>>& responseReader)
        {
            mResponseReader = std::move(responseReader);
        }

        int TopicMessageQuery::ThreadFunction()
        {
          try {
            loguru::set_thread_name(mThreadName.data());
            // copied most of the code from hiero cpp sdk from startSubscription from TopicMessageQuery.cc
            // ::hiero::ConsensusTopicQuery query;
            // Declare needed variables.
            ::grpc::ByteBuffer grpcByteBuffer;
            ::hiero::ConsensusTopicResponse response;
            system_clock::duration backoff = ::hiero::DEFAULT_MIN_BACKOFF;
            system_clock::duration maxBackoff = ::hiero::DEFAULT_MAX_BACKOFF;
            system_clock::time_point lastTransactionTimepoint = system_clock::now();
            ::grpc::Status grpcStatus;
            uint64_t attempt = 0ULL;
            bool complete = false;
            auto completeReason = ConnectionClosedReason::Subscription_Ended;
            bool ok = false;
            void* tag = nullptr;

            // serialize start query into grpc Byte Buffer
            MemoryBlock memoryBuffer(protopuf::serialize<::hiero::ConsensusTopicQuery, ::hiero::ConsensusTopicQueryMessage>(mStartQuery));
            grpcByteBuffer = memoryBuffer.createGrpcBuffer();

            while (!mExitCalled) {
              // Process based on the completion queue status.
              auto backOffFromNow = system_clock::now() + duration_cast<milliseconds>(backoff);
              auto nextStatus = mCompletionQueues.back()->AsyncNext(&tag, &ok, backOffFromNow);
              LOG_F(2, "next status: %s", enum_name(nextStatus).data());
              switch (nextStatus)
              {
              case ::grpc::CompletionQueue::TIMEOUT:
              {
                // Backoff if the completion queue timed out.
                backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
                LOG_F(2, "new timeout: %s", timespanToString(backoff).data());
                if (system_clock::now() - lastTransactionTimepoint > ::hiero::DEFAULT_SUBSCRIPTION_INACTIVE_TIMEOUT) {
                  completeReason = ConnectionClosedReason::Timeout;
                  complete = true;
                  mCompletionQueues.back()->Shutdown();
                  LOG_F(2, "shutdown connection after DEFAULT_SUBSCRIPTION_INACTIVE_TIMEOUT");
                }
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
                    lastTransactionTimepoint = system_clock::now();
                    // Read the response.
                    auto block = MemoryBlock(grpcByteBuffer);
                    LOG_F(INFO, "response: echo \"%s\" | xxd -r -p | protoscope", block.get()->convertToHex().data());
                    response = protopuf::deserialize<::hiero::ConsensusTopicResponse, ::hiero::ConsensusTopicResponseMessage>(*block.get());

                    // Adjust the query timestamp and limit, in case a retry is triggered.
                    auto consensusTimestamp = response.getConsensusTimestamp();
                    if (!consensusTimestamp.empty())
                    {
                      // Add one of the smallest denomination of time
                      mStartQuery.setConsensusStartTime({
                          consensusTimestamp.getSeconds(), consensusTimestamp.getNanos() + 1
                        });
                    }

                    if (mStartQuery.getLimit() > 0ULL)
                    {
                      mStartQuery.setLimit(mStartQuery.getLimit() - 1ULL);
                    }

                    // Process the received message.
                    const auto& chunkInfo = response.getChunkInfo();
                    if (chunkInfo.empty() || chunkInfo.getTotal() == 1)
                    {
                      try {
                        onMessageArrived(std::move(response));
                      }
                      catch (GradidoBlockchainException& ex) {
                        LOG_F(ERROR, "error calling onMessageArrived: %s", ex.getFullString().c_str());
                      }
                      catch (std::exception& ex) {
                        LOG_F(ERROR, "std error calling onMessageArrived: %s", ex.what());
                      }
                      catch (...) {
                        LOG_F(ERROR, "unknown error calling onMessageArrived");
                      }
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
                    mCompletionQueues.back()->Shutdown();

                    // Mark the RPC as complete.
                    complete = true;
                    completeReason = ConnectionClosedReason::Subscription_Ended;
                    break;
                  }
                  else
                  {
                    // An error occurred. Whether retrying or not, cancel the call and close the queue.
                    mClientContexts.back()->TryCancel();
                    mCompletionQueues.back()->Shutdown();

                    if (attempt >= ::hiero::DEFAULT_MAX_ATTEMPTS || !shouldRetry(grpcStatus))
                    {
                      // This RPC call shouldn't be retried, handle the error and mark as complete to exit.
                      //errorHandler(grpcStatus);
                      LOG_F(ERROR, "Subscription error: %s", grpcStatus.error_message().data());
                      complete = true;
                      completeReason = ConnectionClosedReason::Error;
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
                  sleep_for(seconds(1));
                  LOG_F(INFO, "RPC Subscription for topic %s ended.", mStartQuery.getTopicId().toString().data());
                  onConnectionClosed(completeReason);
                  return 0;
                }

                // If the completion queue has been shut down and the RPC hasn't completed, that means the RPC needs to be
                // retried. Increase the backoff for the retry.
                backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
                sleep_for(backoff);
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
                if (!Application::getStopToken().stop_requested()) {
                  mCallStatus = CallStatus::STATUS_CREATE;
                  mCompletionQueues.push_back(make_unique<grpc::CompletionQueue>());
                  mClientContexts.push_back(make_unique<grpc::ClientContext>());
                  MemoryBlock memoryBuffer(protopuf::serialize<::hiero::ConsensusTopicQuery, ::hiero::ConsensusTopicQueryMessage>(mStartQuery));
                  grpcByteBuffer = memoryBuffer.createGrpcBuffer();
                  ServerGlobals::g_HieroMirrorNode->subscribeTopic(this);
                  onConnectionClosed(ConnectionClosedReason::Reconnect);
                }
                break;
              }
              default:
              {
                // Not sure what to do here, just fail out.
                std::cout << "Unknown gRPC completion queue event, failing.." << std::endl;
                onConnectionClosed(ConnectionClosedReason::Error);
                return -1;
              }
              }
            }
          }
          catch (GradidoBlockchainException& ex) {
            LOG_F(ERROR, "Thread has uncaught gradido blockchain exception: %s", ex.getFullString().c_str());
            onConnectionClosed(ConnectionClosedReason::Exception);
            return -3;
          }
          catch (std::exception& ex) {
            LOG_F(ERROR, "Thread has uncaught exception: %s", ex.what());
            onConnectionClosed(ConnectionClosedReason::Exception);
            return -3;
          }
          catch (...) {
            LOG_F(ERROR, "Thread has uncaught unknown exception");
            onConnectionClosed(ConnectionClosedReason::Exception);
            return -3;
          }

          onConnectionClosed(ConnectionClosedReason::Deconstruct);
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
