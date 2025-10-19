#ifndef __GRADIDO_NODE_HIERO_QUERY_H
#define __GRADIDO_NODE_HIERO_QUERY_H

#include "TransactionGetReceiptQuery.h"
#include "ConsensusGetTopicInfoQuery.h"
#include "QueryHeader.h"

namespace hiero {
    // currently only used queries, but there are many more defined in proto
    using QueryMessage = message<
        message_field<"transactionGetReceipt", 14, TransactionGetReceiptQueryMessage>,
        message_field<"consensusGetTopicInfo", 50, ConsensusGetTopicInfoQueryMessage>
    >;

    class Query
    {
    public:
        Query();
        Query(const TransactionGetReceiptQuery& transactionGetReceiptQuery);
        Query(const ConsensusGetTopicInfoQuery& consensusGetTopicInfoQuery);
        Query(const QueryMessage& message);
        ~Query();

        QueryMessage getMessage() const;
        inline const TransactionGetReceiptQuery& getTransactionGetReceiptQuery() const { return *mTransactionGetReceiptQuery; }
        inline const ConsensusGetTopicInfoQuery& getConsensusGetTopicInfoQuery() const { return *mConsensusGetTopicInfoQuery; }
        
        static constexpr size_t maxSize() { return TransactionGetReceiptQuery::maxSize() + 2; }
    protected:
        std::unique_ptr<TransactionGetReceiptQuery> mTransactionGetReceiptQuery;
        std::unique_ptr<ConsensusGetTopicInfoQuery> mConsensusGetTopicInfoQuery;
    };
}
#endif //__GRADIDO_NODE_HIERO_QUERY_H
