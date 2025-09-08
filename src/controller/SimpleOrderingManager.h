#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER

#include "gradido_blockchain/data/GradidoTransaction.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/crypto/SignatureOctet.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "../task/Thread.h"
#include "../hiero/ConsensusTopicResponse.h"

#include <map>
#include <mutex>
#include <chrono>

namespace task {
    class HieroMessageToTransactionTask;
}

namespace controller {

    /*!
     * @author einhornimmond
     *
     * @date 28.08.2025
     *
     * @brief Get transactions from hiero, using multiple connections so it is to expect that every transaction came in multiple times
     *
     * Additional check for cross-group transactions
     */

    class SimpleOrderingManager : public task::Thread
    {
    public:
        SimpleOrderingManager(std::string_view communityId);
        ~SimpleOrderingManager();

        void init(uint64_t lastKnownSequenceNumber);

        enum class PushResult {
            FOUND_IN_LAST_TRANSACTIONS,
            FOUND_IN_TRANSACTIONS,
            ADDED,
            IN_SHUTDOWN
        };
        PushResult pushTransaction(hiero::ConsensusTopicResponse&& consensusTopicResponse);
        std::shared_ptr<task::HieroMessageToTransactionTask> findCrossGroupTransactionPair(const hiero::TransactionId& transactionId) const;

    protected:        

        inline uint64_t getLastSequenceNumber() const { return mLastSequenceNumber; }
        void updateSequenceNumber(uint64_t newSequenceNumber);

        // called from thread
        int ThreadFunction();

        struct TopicResponseDeserializer
        {
            TopicResponseDeserializer(
                hiero::ConsensusTopicResponse&& _consensusTopicResponse,
                std::shared_ptr<task::HieroMessageToTransactionTask> _deserializeTask
            ) : consensusTopicResponse(std::move(_consensusTopicResponse)), 
                deserializeTask(_deserializeTask), 
                putIntoListTime(std::chrono::system_clock::now())
            {
            }

            TopicResponseDeserializer(TopicResponseDeserializer&& move) noexcept
                : consensusTopicResponse(std::move(move.consensusTopicResponse)), 
                deserializeTask(std::move(move.deserializeTask)),
                putIntoListTime(std::move(move.putIntoListTime))
            {
            }

            TopicResponseDeserializer(const TopicResponseDeserializer& other) noexcept
                : consensusTopicResponse(other.consensusTopicResponse), 
                deserializeTask(other.deserializeTask),
                putIntoListTime(other.putIntoListTime)
            {

            }

            ~TopicResponseDeserializer() {}

            hiero::ConsensusTopicResponse consensusTopicResponse;
            std::shared_ptr<task::HieroMessageToTransactionTask> deserializeTask;
            Timepoint putIntoListTime;
        };

        void processTransaction(const TopicResponseDeserializer& gradidoTransactionWorkData);

        // fast duplication check
        // use first 4 and last 4 Byte of transaction hash as key
        // data part is consensusTimestamp which already should be unique.. but better make sure it is really unique
        ExpireCache<SignatureOctet, gradido::data::Timestamp> mLastTransactions;

        std::multimap<gradido::data::Timestamp, TopicResponseDeserializer> mTransactions;
        mutable std::mutex mTransactionsMutex;
        std::string mCommunityId;

        uint64_t mLastSequenceNumber;
    };
}
#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
