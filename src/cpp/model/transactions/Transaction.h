#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H

#include "TransactionBase.h"
#include "TransactionBody.h"
#include "Poco/DateTime.h"
#include "../../proto/gradido/Transaction.pb.h"

namespace model {
	class Transaction : public TransactionBase
	{
	public:
		Transaction(const std::string& transactionBinString);
		~Transaction();

		inline int getID() const { return mProtoTransaction.id(); }
		inline const std::string& getTxHash() const { return mProtoTransaction.txhash(); }
		inline Poco::DateTime getReceived() const { return Poco::Timestamp(mProtoTransaction.received().seconds()); }

		inline TransactionBody* getTransactionBody() { return mTransactionBody; }

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		bool validate(Poco::AutoPtr<Transaction> previousTransaction);
	protected:
		model::messages::gradido::Transaction mProtoTransaction;
		TransactionBody* mTransactionBody;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H