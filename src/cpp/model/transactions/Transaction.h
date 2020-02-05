#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H

#include "TransactionBase.h"
#include "../../proto/gradido/Transaction.pb.h"

namespace model {
	class Transaction : public TransactionBase
	{
	public:
		Transaction(const std::string& transactionBinString);

		inline int getID() const { return mProtoTransaction.id(); }
	protected:
		model::messages::gradido::Transaction mProtoTransaction;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H