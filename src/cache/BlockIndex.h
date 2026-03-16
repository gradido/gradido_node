#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "gradido_blockchain/blockchain/CompactFilter.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/blockchain/TransactionsIndexRoaringBitmaps.h"
#include "gradido_blockchain/crypto/ByteArray.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"

#include "../blockchain/NodeTransactionEntry.h"
#include "../model/files/BlockIndex.h"
#include "../task/CPUTask.h"

#include "rapidjson/document.h"

#include <vector>
#include <map>
#include <mutex>

namespace gradido {
	namespace data::compact {
		class ConfirmedGradidoTx;
	}
	namespace blockchain {
		class AbstractProvider;
		class CompactFilter;
	}
}

namespace cache {

	/*!
	 * @author Dario Rekowski
	 * @date 2020-02-12
	 * @brief store block index in memory for fast finding transactions
	 *
	 * map: uint64 transaction nr, uint32 file cursor
	 * map: uint32 address index, uint64 transaction nr
	 * year[month[
	 TODO: Auto-Recover if missing, and maybe check with saved block on startup
	 */

	class BlockIndex : public gradido::blockchain::TransactionsIndexRoaringBitmaps
	{
		// friend model::files::BlockIndex;
	public:
		BlockIndex(std::string_view groupFolderPath, uint32_t blockNr, uint32_t blockchainCommunityIdIndex);
		~BlockIndex();

		bool init(const IDictionary<PublicKey>& publicKeysDictionary);
		void exit();
		void reset();

		//! \brief loading block index from file (or at least try to load)
		bool loadFromFile(const IDictionary<PublicKey>& publicKeysDictionary);

		//! \brief write block index into files
		std::unique_ptr<model::files::BlockIndex> serialize();

		//! \brief
		//! \return true if there was something to write into file, after writing it to file
		bool writeIntoFile();

		bool addIndicesForTransaction(const gradido::data::compact::ConfirmedGradidoTx& compactTx, const IDictionary<PublicKey>& publicKeyDict);

		//! \brief add transactionNr - fileCursor pair to map if not already exist
		//! \return false if transactionNr exist, else return true
		bool addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor);

		//! \brief search transaction nrs for search criteria in filter, ignore filter function
		//! \return transaction nrs

		inline std::vector<uint64_t> findTransactions(const gradido::blockchain::CompactFilter& filter) const;

		//! count all, ignore pagination
		inline size_t countTransactions(const gradido::blockchain::CompactFilter& filter) const;

		//! \param fileCursor reference to be filled with fileCursor
		//! \return true if transaction nr was found and fileCursor was set, else return false
		bool getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor) const;
		inline bool hasTransactionNr(uint64_t transactionNr) const;

		inline uint64_t getMaxTransactionNr() const;
		inline uint64_t getMinTransactionNr() const;
		inline uint64_t getTransactionsCount() const;

		inline date::year_month getOldestYearMonth() const;
		inline date::year_month getNewestYearMonth() const;
		inline TimepointInterval filteredTimepointInterval(const gradido::blockchain::CompactFilter& filter) const;
		inline void lock() const { mRecursiveMutex.lock(); }
		inline void unlock() const { mRecursiveMutex.unlock(); }

	protected:

		//! \brief called from model::files::BlockIndex while reading file
		std::string				 mFolderPath;
		uint32_t					 mBlockNr;
		uint32_t					 mBlockchainCommunityIdIndex;
		
		std::map<uint64_t, int32_t> mTransactionNrsFileCursors;
		typedef std::pair<uint64_t, int32_t> TransactionNrsFileCursorsPair;

		mutable std::recursive_mutex mRecursiveMutex;
		bool mDirty;
		
	};

	std::vector<uint64_t> BlockIndex::findTransactions(const gradido::blockchain::CompactFilter& filter) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndexRoaringBitmaps::findTransactions(filter);
	}

	size_t BlockIndex::countTransactions(const gradido::blockchain::CompactFilter& filter) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndexRoaringBitmaps::countTransactions(filter);
	}

	size_t BlockIndex::getTransactionsCount() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mMaxTransactionNr && !mMinTransactionNr) {
			return 0;
		}
		return mMaxTransactionNr - mMinTransactionNr + 1;
	}

	bool BlockIndex::hasTransactionNr(uint64_t transactionNr) const
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		return transactionNr >= mMinTransactionNr 
			&& transactionNr <= mMaxTransactionNr; 
	}

	uint64_t BlockIndex::getMaxTransactionNr() const 
	{ 
		std::lock_guard _lock(mRecursiveMutex);  
		return gradido::blockchain::TransactionsIndexRoaringBitmaps::getMaxTransactionNr();
	}
	uint64_t BlockIndex::getMinTransactionNr() const 
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndexRoaringBitmaps::getMinTransactionNr();
	}
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
