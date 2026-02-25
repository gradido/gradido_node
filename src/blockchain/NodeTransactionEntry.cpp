#include "NodeTransactionEntry.h"
#include "FileBased.h"

#include "gradido_blockchain/data/adapter/publicKey.h"

namespace gradido {
	using data::adapter::toPublicKey;

	namespace blockchain {

		NodeTransactionEntry::NodeTransactionEntry(
			gradido::data::ConstConfirmedTransactionPtr transaction,
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transaction, blockchain->getCommunityIdIndex()), mFileCursor(fileCursor)
		{
			auto involvedPublicKeys = transaction->getInvolvedAddresses();
			mPublicKeyIndices.reserve(involvedPublicKeys.size());
			for (auto& publicKey : involvedPublicKeys) {
				mPublicKeyIndices.push_back(blockchain->getOrAddIndexForPublicKey(toPublicKey(publicKey)));
			}
		}


		NodeTransactionEntry::NodeTransactionEntry(
			uint64_t transactionNr,
			date::month month,
			date::year year,
			gradido::data::TransactionType transactionType,
			std::optional<uint32_t> coinCommunityIdIndex,
			const uint32_t* addressIndices,
			uint8_t addressIndiceCount,
			uint32_t blockchainCommunityIdIndex,
			int32_t fileCursor /*= -10*/
		) : TransactionEntry(transactionNr, month, year, transactionType, coinCommunityIdIndex, blockchainCommunityIdIndex), mFileCursor(fileCursor)
		{
			mPublicKeyIndices.reserve(addressIndiceCount);
			for (int i = 0; i < addressIndiceCount; i++) {
				mPublicKeyIndices.push_back(addressIndices[i]);
			}
		}

		NodeTransactionEntry::NodeTransactionEntry(
			memory::ConstBlockPtr serializedTransaction,
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
			int32_t fileCursor/* = -10 */
		) : TransactionEntry(serializedTransaction, blockchain->getCommunityIdIndex()), mFileCursor(fileCursor)
		{
			auto involvedPublicKeys = getConfirmedTransaction()->getInvolvedAddresses();
			mPublicKeyIndices.reserve(involvedPublicKeys.size());
			for (auto& publicKey : involvedPublicKeys) {
				mPublicKeyIndices.push_back(blockchain->getOrAddIndexForPublicKey(toPublicKey(publicKey)));
			}
		}

		NodeTransactionEntry::NodeTransactionEntry(
			gradido::data::ConstConfirmedTransactionPtr transaction,
			memory::ConstBlockPtr serializedTransaction,
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
			int32_t fileCursor/* = -10 */
		) : TransactionEntry(serializedTransaction, transaction, blockchain->getCommunityIdIndex()), mFileCursor(fileCursor)
		{

		}
	}
}
