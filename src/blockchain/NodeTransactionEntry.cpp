#include "NodeTransactionEntry.h"

#include "FileBased.h"

namespace gradido {
	namespace blockchain {

		NodeTransactionEntry::NodeTransactionEntry(
			gradido::data::ConstConfirmedTransactionPtr transaction,
			std::shared_ptr<gradido::blockchain::FileBased> blockchain,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transaction), mFileCursor(fileCursor)
		{
			auto involvedPublicKeys = transaction->getGradidoTransaction()->getInvolvedAddresses();
			mPublicKeyIndices.reserve(involvedPublicKeys.size());
			for (auto& publicKey : involvedPublicKeys) {
				mPublicKeyIndices.push_back(blockchain->getOrAddIndexForPublicKey(publicKey));
			}
		}


		NodeTransactionEntry::NodeTransactionEntry(
			uint64_t transactionNr,
			date::month month,
			date::year year,
			gradido::data::TransactionType transactionType,
			const std::string& coinGroupId,
			const uint32_t* addressIndices,
			uint8_t addressIndiceCount,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transactionNr, month, year, transactionType, coinGroupId), mFileCursor(fileCursor)
		{
			mPublicKeyIndices.reserve(addressIndiceCount);
			for (int i = 0; i < addressIndiceCount; i++) {
				mPublicKeyIndices.push_back(addressIndices[i]);
			}
		}

		NodeTransactionEntry::NodeTransactionEntry(
			memory::ConstBlockPtr serializedTransaction,
			std::shared_ptr<gradido::blockchain::FileBased> blockchain,
			int32_t fileCursor = -10
		) : TransactionEntry(serializedTransaction), mFileCursor(fileCursor)
		{
			auto involvedPublicKeys = getConfirmedTransaction()->getGradidoTransaction()->getInvolvedAddresses();
			mPublicKeyIndices.reserve(involvedPublicKeys.size());
			for (auto& publicKey : involvedPublicKeys) {
				mPublicKeyIndices.push_back(blockchain->getOrAddIndexForPublicKey(publicKey));
			}
		}

	}
}
