#include "TransactionEntry.h"

#include "../model/transactions/Transaction.h"

namespace model {
	TransactionEntry::TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount/* = 2*/)
		: mTransactionNr(_transactionNr), mSerializedTransaction(_serializedTransaction), mFileCursor(-10),
		mMonth(received.month()), mYear(received.year())
	{
		mAddressIndices.reserve(addressIndexCount);
	}

	TransactionEntry::TransactionEntry(std::string _serializedTransaction, uint32_t fileCursor, controller::AddressIndex* addressIndex)
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
}