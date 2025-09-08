#include "Query.h"

namespace hiero {
	Query::Query()
	{

	}

	Query::Query(const TransactionGetReceiptQuery& transactionGetReceiptQuery)
		: mTransactionGetReceiptQuery(std::make_unique<TransactionGetReceiptQuery>(transactionGetReceiptQuery))
	{

	}

	Query::Query(const ConsensusGetTopicInfoQuery& consensusGetTopicInfoQuery)
		: mConsensusGetTopicInfoQuery(std::make_unique<ConsensusGetTopicInfoQuery>(consensusGetTopicInfoQuery))
	{

	}

	Query::Query(const QueryMessage& message)
	{
		if (message["transactionGetReceipt"_f].has_value()) {
			mTransactionGetReceiptQuery = std::make_unique<TransactionGetReceiptQuery>(message["transactionGetReceipt"_f].value());
		}
		else if (message["consensusGetTopicInfo"_f].has_value()) {
			mConsensusGetTopicInfoQuery = std::make_unique<ConsensusGetTopicInfoQuery>(message["consensusGetTopicInfo"_f].value());
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
		else if (mConsensusGetTopicInfoQuery) {
			message["consensusGetTopicInfo"_f] = mConsensusGetTopicInfoQuery->getMessage();
		}
		return message;
	}
}