#ifndef __GRADIDO_NODE_BLOCKCHAIN_NODE_TRANSACTION_ENTRY_H
#define __GRADIDO_NODE_BLOCKCHAIN_NODE_TRANSACTION_ENTRY_H

#include "gradido_blockchain/blockchain/TransactionEntry.h"

#include <vector>

namespace gradido {
	namespace blockchain {

		class FileBased;

		/*!
		* @author einhornimmond
		* @date 2020-02-27
		* @brief container for transaction + index details
		*
		* Used between multiple controller while write to block is pending and while it is cached
		* It contains the serialized transaction and multiple data for fast indexing
		*/
		class NodeTransactionEntry : public TransactionEntry
		{
		public:
			NodeTransactionEntry()
				: mFileCursor(0) {}

			NodeTransactionEntry(
				gradido::data::ConstConfirmedTransactionPtr transaction,
				std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
				int32_t fileCursor = -10
			);

			NodeTransactionEntry(
				memory::ConstBlockPtr serializedTransaction,
				std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
				int32_t fileCursor = -10
			);

			//! \brief init entry object from details e.g. by loading from file
			NodeTransactionEntry(
				uint64_t transactionNr,
				date::month month,
				date::year year,
				gradido::data::TransactionType transactionType,
				const std::string& coinGroupId,
				const uint32_t* addressIndices, uint8_t addressIndiceCount,
				int32_t fileCursor = -10
			);

			inline void setFileCursor(int32_t newFileCursorValue) { std::scoped_lock lock(mFastMutex); mFileCursor = newFileCursorValue; }
			inline int32_t getFileCursor() const { std::scoped_lock lock(mFastMutex); return mFileCursor; }

			inline void addAddressIndex(uint32_t addressIndex) { std::scoped_lock lock(mFastMutex); mPublicKeyIndices.push_back(addressIndex); }
			inline const std::vector<uint32_t>& getAddressIndices() const { std::scoped_lock lock(mFastMutex); return mPublicKeyIndices; }
			inline bool isAddressIndexInvolved(uint32_t addressIndex) const; 

		protected:
			int32_t mFileCursor;
			std::vector<uint32_t> mPublicKeyIndices;
		};

		bool NodeTransactionEntry::isAddressIndexInvolved(uint32_t addressIndex) const
		{
			std::scoped_lock lock(mFastMutex);
			return std::find(mPublicKeyIndices.begin(), mPublicKeyIndices.end(), addressIndex) != mPublicKeyIndices.end();
		}
	}
};

#endif //__GRADIDO_NODE_BLOCKCHAIN_NODE_TRANSACTION_ENTRY_H
