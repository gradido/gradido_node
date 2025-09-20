#include "SubscriptionThread.h"
#include "const.h"
#include <chrono>

namespace client {
	namespace hiero {

        // Enum to track the status of the gRPC call.
        enum class CallStatus
        {
            STATUS_CREATE = 0,
            STATUS_PROCESSING = 1,
            STATUS_FINISH = 2
        };

        int SubscriptionThread::ThreadFunction() 
        {
            /*
            // copied most of the code from hiero cpp sdk from startSubscription from TopicMessageQuery.cc
            std::chrono::system_clock::duration backoff = ::hiero::DEFAULT_MIN_BACKOFF;
            std::chrono::system_clock::duration maxBackoff = ::hiero::DEFAULT_MAX_BACKOFF;
            auto callStatus = std::make_unique<CallStatus>(CallStatus::STATUS_CREATE);
            bool ok = false;
            void* tag = nullptr;
            while (true) {
                // Process based on the completion queue status.
                auto backOffFromNow = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::milliseconds>(backoff);
                switch (mCompletionQueue.AsyncNext(&tag, &ok, backOffFromNow))
                {
                case ::grpc::CompletionQueue::TIMEOUT:
                {
                    // Backoff if the completion queue timed out.
                    backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
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
                            // Update the call status to processing.
                            *callStatus = CallStatus::STATUS_PROCESSING;
                        }

                        // Let fall through to process message.
                        [[fallthrough]];
                    }
                    case CallStatus::STATUS_PROCESSING:
                    {
                        // If the response should be processed, process it.
                        if (ok)
                        {
                            // Read the response.
                            // mResponseReader->
                            reader->Read(&response, callStatus.get());

                            // Adjust the query timestamp and limit, in case a retry is triggered.
                            if (response.has_consensustimestamp())
                            {
                                // Add one of the smallest denomination of time this machine can handle.
                                query.set_allocated_consensusstarttime(internal::TimestampConverter::toProtobuf(
                                    internal::TimestampConverter::fromProtobuf(response.consensustimestamp()) +
                                    std::chrono::duration<int, std::ratio<1, std::chrono::system_clock::period::den>>()));
                            }

                            if (query.limit() > 0ULL)
                            {
                                query.set_limit(query.limit() - 1ULL);
                            }

                            // Process the received message.
                            if (!response.has_chunkinfo() || response.chunkinfo().total() == 1)
                            {
                                onNext(TopicMessage::ofSingle(response));
                            }
                            else
                            {
                                const TransactionId transactionId =
                                    TransactionId::fromProtobuf(response.chunkinfo().initialtransactionid());
                                pendingMessages[transactionId].push_back(response);

                                if (pendingMessages[transactionId].size() == response.chunkinfo().total())
                                {
                                    onNext(TopicMessage::ofMany(pendingMessages[transactionId]));
                                }
                            }
                        }

                        // If the response shouldn't be processed (due to completion or error), finish the RPC.
                        else
                        {
                            *callStatus = CallStatus::STATUS_FINISH;
                            reader->Finish(&grpcStatus, callStatus.get());
                        }

                        break;
                    }
                    case CallStatus::STATUS_FINISH:
                    {
                        if (grpcStatus.ok())
                        {
                            // RPC completed successfully.
                            completionHandler();

                            // Shutdown the completion queue.
                            queues.back()->Shutdown();

                            // Mark the RPC as complete.
                            complete = true;
                            break;
                        }
                        else
                        {
                            // An error occurred. Whether retrying or not, cancel the call and close the queue.
                            contexts.back()->TryCancel();
                            queues.back()->Shutdown();

                            if (attempt >= maxAttempts || !retryHandler(grpcStatus))
                            {
                                // This RPC call shouldn't be retried, handle the error and mark as complete to exit.
                                errorHandler(grpcStatus);
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
                case grpc::CompletionQueue::SHUTDOWN:
                {
                    // Getting here means the RPC is reached completion or encountered an un-retriable error, and the completion
                    // queue has been shut down. End the subscription.
                    if (complete)
                    {
                        // Give a second for the queue to finish its processing.
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        return;
                    }

                    // If the completion queue has been shut down and the RPC hasn't completed, that means the RPC needs to be
                    // retried. Increase the backoff for the retry.
                    backoff = (backoff * 2 > maxBackoff) ? maxBackoff : backoff * 2;
                    std::this_thread::sleep_for(backoff);
                    ++attempt;

                    // Resend the query to a different node with a different completion queue and client context.
                    contexts.push_back(std::make_unique<grpc::ClientContext>());
                    queues.push_back(std::make_unique<grpc::CompletionQueue>());

                    // Reset the call status and send the query.
                    *callStatus = CallStatus::STATUS_CREATE;
                    reader = getConnectedMirrorNode(network)->getConsensusServiceStub()->AsyncsubscribeTopic(
                        contexts.back().get(), query, queues.back().get(), callStatus.get());

                    break;
                }
                default:
                {
                    // Not sure what to do here, just fail out.
                    std::cout << "Unknown gRPC completion queue event, failing.." << std::endl;
                    return;
                }
                }
            }
            */
            return 0;
        }

	}
}
