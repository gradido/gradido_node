#include "TransactionGetReceiptResponse.h"

namespace hiero {
	TransactionGetReceiptResponse::TransactionGetReceiptResponse()
	{

	}

	TransactionGetReceiptResponse::TransactionGetReceiptResponse(const TransactionGetReceiptResponseMessage& message)
		: mHeader(message["header"_f].value()), mReceipt(message["receipt"_f].value())
	{
		auto duplicateTransactionReceiptsSize = message["duplicateTransactionReceipts"_f].size();
		mDuplicateTransactionReceipts.reserve(duplicateTransactionReceiptsSize);

		for (int i = 0; i < duplicateTransactionReceiptsSize; i++) {
			mDuplicateTransactionReceipts.push_back(message["duplicateTransactionReceipts"_f][i]);
		}
	}

	TransactionGetReceiptResponse::~TransactionGetReceiptResponse()
	{

	}

	TransactionGetReceiptResponseMessage TransactionGetReceiptResponse::getMessage() const
	{
		TransactionGetReceiptResponseMessage message;
		message["header"_f] = mHeader.getMessage();
		message["receipt"_f] = mReceipt.getMessage();
		if (mDuplicateTransactionReceipts.size()) {
			message["duplicateTransactionReceipts"_f].reserve(mDuplicateTransactionReceipts.size());
			for (const auto& receipt : mDuplicateTransactionReceipts) {
				message["duplicateTransactionReceipts"_f].push_back(receipt.getMessage());
			}
		}
		return message;
	}
}