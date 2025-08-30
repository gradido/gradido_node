#ifndef __GRADIDO_NODE_HIERO_QUERY_H
#define __GRADIDO_NODE_HIERO_QUERY_H

#include "TransactionGetReceiptQuery.h"
#include "QueryHeader.h"

namespace hiero {
    // currently only used queries, but there are many more defined in proto
    using QueryMessage = message<
        message_field<"transactionGetReceipt", 14, TransactionGetReceiptQueryMessage>
    >;

    class Query
    {
    public:
        Query();
        Query(std::shared_ptr<TransactionGetReceiptQuery> transactionGetReceiptQuery);
        Query(const QueryMessage& message);
        ~Query();

        QueryMessage getMessage() const;
        inline std::shared_ptr<TransactionGetReceiptQuery> getTransactionGetReceiptQuery() const { return mTransactionGetReceiptQuery; }
        
        static constexpr size_t maxSize() { return TransactionGetReceiptQuery::maxSize() + 2; }
    protected:
        std::shared_ptr<TransactionGetReceiptQuery> mTransactionGetReceiptQuery;
    };
}
#endif //__GRADIDO_NODE_HIERO_QUERY_H
