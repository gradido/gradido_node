#include "NodeTransactionEntry.h"


namespace gradido {
	namespace blockchain {

		NodeTransactionEntry::NodeTransactionEntry(
			gradido::data::ConstConfirmedTransactionPtr transaction,
			cache::AddressIndex& addressIndex,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transaction), mFileCursor(fileCursor)
		{
			auto addresses = transaction->getGradidoTransaction()->getInvolvedAddresses();
			if (addresses.size()) {
				mAddressIndices = addressIndex.getOrAddIndicesForAddresses(addresses);
			}
		}


		NodeTransactionEntry::NodeTransactionEntry(
			uint64_t transactionNr,
			date::month month,
			date::year year,
			gradido::data::TransactionType transactionType,
			const std::string& coinGroupId,
			const uint32_t* addressIndices, uint8_t addressIndiceCount
		) : TransactionEntry(transactionNr, month, year, transactionType, coinGroupId)
		{
			mAddressIndices.reserve(addressIndiceCount);
			for (int i = 0; i < addressIndiceCount; i++) {
				mAddressIndices.push_back(addressIndices[i]);
			}
		}

	}
}
