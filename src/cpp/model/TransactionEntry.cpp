#include "TransactionEntry.h"

namespace model {
	TransactionEntry::TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount/* = 2*/)
		: mTransactionNr(_transactionNr), mSerializedTransaction(_serializedTransaction), mFileCursor(-10),
		mMonth(received.month()), mYear(received.year())
	{
		mAddressIndices.reserve(addressIndexCount);
	}
}