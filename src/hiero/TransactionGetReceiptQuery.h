#ifndef __GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_QUERY_H
#define __GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_QUERY_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "QueryHeader.h"

namespace hiero {
    using TransactionGetReceiptQueryMessage = message<
        message_field<"header", 1, QueryHeaderMessage>,
        message_field<"transactionID", 2, gradido::interaction::deserialize::HieroTransactionIdMessage>,
        bool_field<"includeDuplicates", 3>,
        bool_field<"include_child_receipts", 4>
    >;

    class TransactionGetReceiptQuery
    {
    public:
        TransactionGetReceiptQuery();
        TransactionGetReceiptQuery(const TransactionGetReceiptQueryMessage& message);
        TransactionGetReceiptQuery(
            const QueryHeader& header, 
            const TransactionId& transactionId, 
            bool includeDuplicates = true, 
            bool includeChildReceipts = false
        );
        ~TransactionGetReceiptQuery();

        TransactionGetReceiptQueryMessage getMessage() const;
        inline const QueryHeader& getHeader() const { return mHeader; }
        inline const TransactionId& getTransactionID() const { return mTransactionId; }
        inline bool isIncludeDuplicates() const { return mIncludeDuplicates; }
        inline bool isIncludeChildReceipts() const { return mIncludeChildReceipts; }

        // don't expect alias for transactionId.accountId
        static constexpr size_t maxSize() { return QueryHeader::maxSize() + 4 + 2 + 62; }

    protected:
        QueryHeader mHeader;
        TransactionId mTransactionId;
        bool mIncludeDuplicates;
        bool mIncludeChildReceipts;
    };
}
#endif //__GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_QUERY_H
