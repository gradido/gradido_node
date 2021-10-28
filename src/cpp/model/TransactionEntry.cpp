#include "TransactionEntry.h"

#include "../model/transactions/GradidoBlock.h"

namespace model {


	TransactionEntry::TransactionEntry(std::string _serializedTransaction, uint32_t fileCursor, Poco::SharedPtr<controller::Group> groupRoot)
		: mTransactionNr(0), mSerializedTransaction(_serializedTransaction), mFileCursor(fileCursor)
	{
		Poco::AutoPtr<model::GradidoBlock> transaction(new model::GradidoBlock(_serializedTransaction, groupRoot));
		if (transaction->errorCount() > 0) {
			throw Poco::Exception("TransactionEntry::TransactionEntry error by loading from serialized transaction");
		}
		mTransactionNr = transaction->getID();
		mMonth = transaction->getReceived().month();
		mYear = transaction->getReceived().year();

		mAddressIndices = transaction->getInvolvedAddressIndices(groupRoot->getAddressIndex());
	}

	TransactionEntry::TransactionEntry(Poco::AutoPtr<GradidoBlock> transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex)
		: mTransactionNr(transaction->getID()), mSerializedTransaction(transaction->getSerialized()), mFileCursor(-10)
	{
		auto received = transaction->getReceived();
		mMonth = received.month();
		mYear = received.year();
		mAddressIndices = transaction->getInvolvedAddressIndices(addressIndex);
	}

	TransactionEntry::TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, const uint32_t* addressIndices, uint8_t addressIndiceCount)
		: TransactionEntry(transactionNr, month, year)
	{
		mAddressIndices.reserve(addressIndiceCount);
		for (int i = 0; i < addressIndiceCount; i++) {
			mAddressIndices.push_back(addressIndices[i]);
		}
	}

	TransactionEntry::TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year)
		: mTransactionNr(transactionNr), mFileCursor(0), mMonth(month), mYear(year)
	{

	}
}
