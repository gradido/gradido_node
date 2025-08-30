#ifndef __GRADIDO_NODE_HIERO_RESPONSE_H
#define __GRADIDO_NODE_HIERO_RESPONSE_H

#include "TransactionGetReceiptResponse.h"

namespace hiero {
    using ResponseMessage = message<
        message_field<"transactionGetReceipt", 14, TransactionGetReceiptResponseMessage>
    >;

    class Response
    {
    public:
        Response();
        Response(std::shared_ptr<TransactionGetReceiptResponse> transactionGetReceiptResponse);
        Response(const ResponseMessage& message);
        ~Response();

        ResponseMessage getMessage() const;
        inline std::shared_ptr<TransactionGetReceiptResponse> getTransactionGetReceiptResponse() const { 
            return mTransactionGetReceiptResponse; 
        }

    protected:
        std::shared_ptr<TransactionGetReceiptResponse> mTransactionGetReceiptResponse;
    };
}
#endif //__GRADIDO_NODE_HIERO_RESPONSE_H
