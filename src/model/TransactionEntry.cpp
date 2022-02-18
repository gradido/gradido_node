#include "TransactionEntry.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

namespace model {

	TransactionEntry::TransactionEntry(std::unique_ptr<std::string> _serializedTransaction, int32_t fileCursor, Poco::SharedPtr<controller::Group> groupRoot)
		: mTransactionNr(0), mSerializedTransaction(std::move(_serializedTransaction)), mFileCursor(fileCursor)
	{
		Poco::AutoPtr<model::gradido::GradidoBlock> transaction(new model::gradido::GradidoBlock(mSerializedTransaction.get()));

		mTransactionNr = transaction->getID();
		mMonth = transaction->getReceived().month();
		mYear = transaction->getReceived().year();
		mCoinColor = transaction->getGradidoTransaction()->getTransactionBody()->getTransactionBase()->getCoinColor();

		auto addresses = transaction->getGradidoTransaction()->getTransactionBody()->getTransactionBase()->getInvolvedAddresses();
		if (addresses.size()) {
			mAddressIndices = groupRoot->getAddressIndex()->getOrAddIndicesForAddresses(addresses);
		}
	}

	TransactionEntry::TransactionEntry(Poco::AutoPtr<gradido::GradidoBlock> transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex)
		: mTransactionNr(transaction->getID()), mSerializedTransaction(transaction->getSerialized()), mFileCursor(-10)
	{
		auto received = transaction->getReceived();
		mMonth = received.month();
		mYear = received.year();
		mCoinColor = transaction->getGradidoTransaction()->getTransactionBody()->getTransactionBase()->getCoinColor();
		auto addresses = transaction->getGradidoTransaction()->getTransactionBody()->getTransactionBase()->getInvolvedAddresses();
		if (addresses.size()) {
			mAddressIndices = addressIndex->getOrAddIndicesForAddresses(addresses);
		}
	}

	TransactionEntry::TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor, const uint32_t* addressIndices, uint8_t addressIndiceCount)
		: TransactionEntry(transactionNr, month, year, coinColor)
	{
		mAddressIndices.reserve(addressIndiceCount);
		for (int i = 0; i < addressIndiceCount; i++) {
			mAddressIndices.push_back(addressIndices[i]);
		}
	}

	TransactionEntry::TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor)
		: mTransactionNr(transactionNr), mFileCursor(-10), mMonth(month), mYear(year), mCoinColor(coinColor)
	{

	}
}
