#include "NodeTransactionEntry.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

namespace model {

	
	NodeTransactionEntry::NodeTransactionEntry(gradido::GradidoBlock* transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex, int32_t fileCursor /*= -10*/)
		: TransactionEntry(transaction), mFileCursor(fileCursor)
	{
		auto transactionBase = transaction->getGradidoTransaction()->getTransactionBody()->getTransactionBase();		
		auto addresses = transactionBase->getInvolvedAddresses();
		if (addresses.size()) {
			mAddressIndices = addressIndex->getOrAddIndicesForAddresses(addresses);
		}
	}


	NodeTransactionEntry::NodeTransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor, const uint32_t* addressIndices, uint8_t addressIndiceCount)
		: TransactionEntry(transactionNr, month, year, coinColor)
	{
		mAddressIndices.reserve(addressIndiceCount);
		for (int i = 0; i < addressIndiceCount; i++) {
			mAddressIndices.push_back(addressIndices[i]);
		}
	}
}
