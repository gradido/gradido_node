#include "Query.h"

namespace hiero {
	Query::Query()
	{

	}

	Query::Query(std::shared_ptr<TransactionGetReceiptQuery> transactionGetReceiptQuery)
		: mTransactionGetReceiptQuery(transactionGetReceiptQuery)
	{

	}

	Query::Query(const QueryMessage& message)
	{
		if (message["transactionGetReceipt"_f].has_value()) {
			mTransactionGetReceiptQuery = std::make_shared<TransactionGetReceiptQuery>(message["transactionGetReceipt"_f].value());
		}
	}

	Query::~Query()
	{

	}

	QueryMessage Query::getMessage() const
	{
		QueryMessage message;
		if (mTransactionGetReceiptQuery) {
			message["transactionGetReceipt"_f] = mTransactionGetReceiptQuery->getMessage();
		}
		return message;
	}

	size_t Query::maxSize() const
	{
		if (mTransactionGetReceiptQuery) {
			return mTransactionGetReceiptQuery->maxSize();
		}
		return 0;
	}
}