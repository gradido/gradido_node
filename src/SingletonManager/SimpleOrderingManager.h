#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER

#include "gradido_blockchain/data/GradidoTransaction.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/crypto/SignatureOctet.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "../task/Thread.h"
#include "../task/HieroMessageToTransactionTask.h"

#include <map>
#include <mutex>
#include <chrono>

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
    ~SimpleOrderingManager();
    static SimpleOrderingManager* getInstance();

    enum class PushResult {
        FOUND_IN_LAST_TRANSACTIONS,
        FOUND_IN_TRANSACTIONS,
        ADDED
    };

    PushResult pushTransaction(
        memory::Block&& transactionHash,
        const hiero::TransactionId transactionId,
        uint64_t hieroSequenceNumber,
        std::shared_ptr<const memory::Block> transactionRaw, 
        const gradido::data::Timestamp& consensusTimestamp,
        const std::string_view communityId
    );

protected:
    // Singleton Protected Constructor
    SimpleOrderingManager();

    uint64_t getLastSequenceNumber(const std::string& communityId) const;
    void updateSequenceNumber(const std::string& communityId, uint64_t newSequenceNumber);

	// called from thread
    int ThreadFunction();

    struct GradidoTransactionWithGroup
    {
        GradidoTransactionWithGroup(
            const hiero::TransactionId& _hieroTransactionId,
            uint64_t _hieroSequenceNumber,
            std::shared_ptr<const memory::Block> _transactionRaw,
            const gradido::data::Timestamp& _consensusTimestamp,
            std::string_view _communityId,
            std::shared_ptr<HieroMessageToTransactionTask> _deserializeTask
        ) : hieroTransactionId(_hieroTransactionId),
            hieroSequenceNumber(_hieroSequenceNumber), 
            transactionRaw(_transactionRaw), 
            consensusTimestamp(_consensusTimestamp), 
            communityId(_communityId),
            deserializeTask(_deserializeTask)
        {
        }

        GradidoTransactionWithGroup(GradidoTransactionWithGroup&& move) noexcept
            : hieroTransactionId(std::move(move.hieroTransactionId)),
            hieroSequenceNumber(move.hieroSequenceNumber),
            transactionRaw(std::move(move.transactionRaw)),
            consensusTimestamp(std::move(move.consensusTimestamp)),
            communityId(std::move(move.communityId)),
            deserializeTask(std::move(move.deserializeTask))
        {
        }

        GradidoTransactionWithGroup(const GradidoTransactionWithGroup& other) noexcept
            : hieroTransactionId(other.hieroTransactionId),
            hieroSequenceNumber(other.hieroSequenceNumber), 
            transactionRaw(other.transactionRaw),
            consensusTimestamp(other.consensusTimestamp),
            communityId(other.communityId),
            deserializeTask(other.deserializeTask) 
        {

        }

        ~GradidoTransactionWithGroup() {}        

        hiero::TransactionId hieroTransactionId;
        uint64_t hieroSequenceNumber;
        std::shared_ptr<const memory::Block> transactionRaw;
        gradido::data::Timestamp consensusTimestamp;        
        std::string communityId;
        std::shared_ptr<HieroMessageToTransactionTask> deserializeTask;
    };

    void processTransaction(const GradidoTransactionWithGroup& gradidoTransactionWorkData);

    // fast duplication check
    // use first 4 and last 4 Byte of transaction hash as key
    // data part is consensusTimestamp which already should be unique.. but better make sure it is really unique
    ExpireCache<SignatureOctet, gradido::data::Timestamp> mLastTransactions;

    std::multimap<gradido::data::Timestamp, GradidoTransactionWithGroup> mTransactions;
    std::mutex mTransactionsMutex;

    // key: communityId, value: last hiero sequence number of topic, used only from thread so no mutex needed here
    std::map<std::string, uint64_t> mGroupAliasLastSequenceNumbers;
    
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
