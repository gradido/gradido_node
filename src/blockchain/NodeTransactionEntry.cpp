#include "NodeTransactionEntry.h"
#include "FileBased.h"

#include "gradido_blockchain_core/memory.h"
#include "gradido_blockchain_core/data/wire/confirmed_transaction.h"
#include "gradido_blockchain_core/data/wire/transaction_body.h"
#include "gradido_blockchain/data/adapter/publicKey.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"

namespace gradido {
	using data::adapter::toPublicKey;
	using data::compact::ConfirmedGradidoTx;

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

		ConfirmedGradidoTx NodeTransactionEntry::convertToCompactConfirmedTx() const
		{
			uint8_t buffer[1024];
			grd_memory alloc;
			grd_memory_init_arena_static(&alloc, buffer, 1024);
			grdw_confirmed_transaction tx{};
			getConfirmedTransaction()->toGrdw(&alloc, &tx, mBlockchainCommunityIdIndex);
			auto confirmedTx = ConfirmedGradidoTx::fromGrdw(&tx, mBlockchainCommunityIdIndex, *g_appContext);
			alloc.last_index = 0;
			grdw_transaction_body txBody{};
			getTransactionBody()->toGrdw(&alloc, &txBody);
			confirmedTx.fillFromGrdwTransactionBody(&txBody, *g_appContext);
			return confirmedTx;
		}
	}
}
