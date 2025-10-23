#include "Response.h"

namespace hiero {
    Response::Response()
    {
    }

    Response::Response(std::shared_ptr<TransactionGetReceiptResponse> transactionGetReceiptResponse)
        : mTransactionGetReceiptResponse(transactionGetReceiptResponse)
    {
    }

    Response::Response(const ResponseMessage& message)
    {
        if (message["transactionGetReceipt"_f].has_value()) {
            mTransactionGetReceiptResponse = std::make_shared<TransactionGetReceiptResponse>(message["transactionGetReceipt"_f].value());
        }
    }

    Response::~Response()
    {
    }

    ResponseMessage Response::getMessage() const
    {
        ResponseMessage message;
        if (mTransactionGetReceiptResponse) {
            message["transactionGetReceipt"_f] = mTransactionGetReceiptResponse->getMessage();
        }
        return message;
    }
}