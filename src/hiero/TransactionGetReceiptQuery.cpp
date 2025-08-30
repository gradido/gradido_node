#include "TransactionGetReceiptQuery.h"
#include "gradido_blockchain/interaction/deserialize/HieroTransactionIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroTransactionIdRole.h"

using namespace gradido::interaction;

namespace hiero {
    TransactionGetReceiptQuery::TransactionGetReceiptQuery()
        : mIncludeDuplicates(true), mIncludeChildReceipts(false)
    {

    }
    TransactionGetReceiptQuery::TransactionGetReceiptQuery(const TransactionGetReceiptQueryMessage& message)
        : mHeader(message["header"_f].value()),
        mTransactionId(deserialize::HieroTransactionIdRole(message["transactionID"_f].value())), 
        mIncludeDuplicates(message["includeDuplicates"_f].value()),
        mIncludeChildReceipts(message["include_child_receipts"_f].value())
    {

    }

    TransactionGetReceiptQuery::TransactionGetReceiptQuery(
        const QueryHeader& header,
        const TransactionId& transactionId,
        bool includeDuplicates/* = true*/,
        bool includeChildReceipts /*= false*/
    ): mHeader(header), mTransactionId(transactionId), mIncludeDuplicates(includeDuplicates), mIncludeChildReceipts(includeChildReceipts)
    {

    }
    TransactionGetReceiptQuery::~TransactionGetReceiptQuery()
    {

    }

    TransactionGetReceiptQueryMessage TransactionGetReceiptQuery::getMessage() const
    {
        return TransactionGetReceiptQueryMessage{
            mHeader.getMessage(),
            serialize::HieroTransactionIdRole(mTransactionId).getMessage(),
            mIncludeDuplicates,
            mIncludeChildReceipts
        };
    }
}