#include "TransactionEntry.h"

#include "../model/transactions/Transaction.h"

namespace model {
	

	TransactionEntry::TransactionEntry(std::string _serializedTransaction, uint32_t fileCursor, Poco::SharedPtr<controller::AddressIndex> addressIndex)
		: mTransactionNr(0), mSerializedTransaction(_serializedTransaction), mFileCursor(fileCursor)
	{
		auto transaction = new model::Transaction(_serializedTransaction);
		if (transaction->errorCount() > 0) {
			throw Poco::Exception("TransactionEntry::TransactionEntry error by loading from serialized transaction");
		}
		mTransactionNr = transaction->getID();
		mMonth = transaction->getReceived().month();
		mYear = transaction->getReceived().year();

		mAddressIndices = transaction->getInvolvedAddressIndices(addressIndex);
	}

	TransactionEntry::TransactionEntry(Poco::AutoPtr<Transaction> transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex)
		: mTransactionNr(transaction->getID()), mSerializedTransaction(transaction->getSerialized()), mFileCursor(-10)
	{
		auto received = transaction->getReceived();
		mMonth = received.month();
		mYear = received.year();
		mAddressIndices = transaction->getInvolvedAddressIndices(addressIndex);
	}
}