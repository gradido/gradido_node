#ifndef __GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_RESPONSE_H
#define __GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_RESPONSE_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "ResponseHeader.h"
#include "TransactionReceipt.h"

namespace hiero {
    using TransactionGetReceiptResponseMessage = message<
        message_field<"header", 1, ResponseHeaderMessage>,
        message_field<"receipt", 2, TransactionReceiptMessage>,
        message_field<"duplicateTransactionReceipts", 4, TransactionReceiptMessage, repeated>
        // A list of receipts for all child transactions spawned by the requested
        // transaction.
        // message_field<"child_transaction_receipts", 5, TransactionReceiptMessage, repeated>
    >;

    class TransactionGetReceiptResponse
    {
    public:
        TransactionGetReceiptResponse();
        TransactionGetReceiptResponse(const TransactionGetReceiptResponseMessage& message);
        ~TransactionGetReceiptResponse();

        TransactionGetReceiptResponseMessage getMessage() const;
        inline const ResponseHeader& getHeader() const { return mHeader; }
        inline const TransactionReceipt& getReceipt() const { return mReceipt; }
        inline const std::vector<TransactionReceipt>& getDuplicateTransactionReceipts() const { return mDuplicateTransactionReceipts; }

    protected:
        /**
         * The standard response information for queries.<br/>
         * This includes the values requested in the `QueryHeader`
         * (cost, state proof, both, or neither).
         */
        ResponseHeader mHeader;

        /**
         * A transaction receipt.
         * <p>
         * This SHALL be the receipt for the "first" transaction that matches
         * the transaction identifier requested.<br/>
         * If the identified transaction has not reached consensus, this receipt
         * SHALL have a `status` of `UNKNOWN`.<br/>
         * If the identified transaction reached consensus prior to the current
         * configured receipt period (typically the last 180 seconds), this receipt
         * SHALL have a `status` of `UNKNOWN`.
         */
        TransactionReceipt mReceipt;

        /**
         * A list of duplicate transaction receipts.
         * <p>
         * If the request set the `includeDuplicates` flat, this list SHALL
         * include the receipts for each duplicate transaction associated to the
         * requested transaction identifier.
         * If the request did not set the `includeDuplicates` flag, this list
         * SHALL be empty.<br/>
         * If the `receipt` status is `UNKNOWN`, this list SHALL be empty.<br/>
         * This list SHALL be in order by consensus timestamp.
         */
        std::vector<TransactionReceipt> mDuplicateTransactionReceipts;
    };
}
#endif //__GRADIDO_NODE_HIERO_TRANSACTION_GET_RECEIPT_RESPONSE_H
