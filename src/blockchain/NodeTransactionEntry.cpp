#include "NodeTransactionEntry.h"


namespace gradido {
	namespace blockchain {

		NodeTransactionEntry::NodeTransactionEntry(
			gradido::data::ConstConfirmedTransactionPtr transaction,
			cache::Dictionary& publicKeyIndex,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transaction), mFileCursor(fileCursor)
		{
			auto involvedPublicKeys = transaction->getGradidoTransaction()->getInvolvedAddresses();
			mPublicKeyIndices.reserve(involvedPublicKeys.size());
			for (auto& publicKey : involvedPublicKeys) {
				mPublicKeyIndices.push_back(publicKeyIndex.getOrAddIndexForString(publicKey->copyAsString()));
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
			mPublicKeyIndices.reserve(addressIndiceCount);
			for (int i = 0; i < addressIndiceCount; i++) {
				mPublicKeyIndices.push_back(addressIndices[i]);
			}
		}

	}
}
