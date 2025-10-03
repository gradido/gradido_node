#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "gradido_blockchain/blockchain/Filter.h"

#include "../blockchain/NodeTransactionEntry.h"
#include "../model/files/BlockIndex.h"
#include "../task/CPUTask.h"

#include "Dictionary.h"

#include <vector>
#include <map>

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

	class BlockIndex : public model::files::IBlockIndexReceiver
	{
		//friend model::files::BlockIndex;
	public:
		BlockIndex(std::string_view groupFolderPath, uint32_t blockNr);
		~BlockIndex();

		bool init();
		void exit();
		void reset();

		//! \brief loading block index from file (or at least try to load)
		bool loadFromFile();

		//! \brief write block index into files
		std::unique_ptr<model::files::BlockIndex> serialize();
		//! \brief
		//! \return true if there was something to write into file, after writing it to file
		bool writeIntoFile();

		bool addIndicesForTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry);

		//! implement from model::files::IBlockIndexReceiver, called by loading block index from file
		bool addIndicesForTransaction(
			gradido::data::TransactionType transactionType,
			uint32_t coinCommunityIdIndex,
			date::year year,
			date::month month,
			uint64_t transactionNr, 
			int32_t fileCursor, 
			const uint32_t* addressIndices,
			uint8_t addressIndiceCount
		);

		//! \brief add transactionNr - fileCursor pair to map if not already exist
		//! \return false if transactionNr exist, else return true
		bool addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor);

		//! \brief search transaction nrs for search criteria in filter, ignore filter function
		//! \return transaction nrs
		std::vector<uint64_t> findTransactions(const gradido::blockchain::Filter& filter, const Dictionary& publicKeysDictionary) const;

		//! count all, ignore pagination
		size_t countTransactions(const gradido::blockchain::Filter& filter, const Dictionary& publicKeysDictionary) const;

		//! \brief find transaction nrs from specific month and year
		//! \return {0, 0} if nothing found
		std::pair<uint64_t, uint64_t> findTransactionsForMonthYear(date::year year, date::month month) const;

		//! \param fileCursor reference to be filled with fileCursor
		//! \return true if transaction nr was found and fileCursor was set, else return false
		bool getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor) const;
		inline bool hasTransactionNr(uint64_t transactionNr) const;

		inline uint64_t getMaxTransactionNr() const { std::lock_guard _lock(mRecursiveMutex);  return mMaxTransactionNr; }
		inline uint64_t getMinTransactionNr() const { std::lock_guard _lock(mRecursiveMutex); return mMinTransactionNr; }
		inline uint64_t getTransactionsCount() const;

		date::year_month getOldestYearMonth() const;
		date::year_month getNewestYearMonth() const;

	protected:
		void clearIndexEntries(); 

		//! \brief called from model::files::BlockIndex while reading file
		std::string				 mFolderPath;
		uint32_t				 mBlockNr;
		uint64_t				 mMaxTransactionNr;
		uint64_t				 mMinTransactionNr;

		std::map<uint64_t, int32_t> mTransactionNrsFileCursors;
		typedef std::pair<uint64_t, int32_t> TransactionNrsFileCursorsPair;

		struct BlockIndexEntry
		{
			uint64_t						transactionNr;
			uint32_t*						addressIndices;
			uint32_t						coinCommunityIdIndex;
			gradido::data::TransactionType	transactionType;
			uint8_t							addressIndiceCount;
		};

		std::map<date::year, std::map<date::month, std::list<BlockIndexEntry>>> mYearMonthAddressIndexEntrys;

		mutable std::recursive_mutex mRecursiveMutex;
		bool mDirty;
	};

	bool BlockIndex::hasTransactionNr(uint64_t transactionNr) const
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		return transactionNr >= mMinTransactionNr 
			&& transactionNr <= mMaxTransactionNr; 
	}

	uint64_t BlockIndex::getTransactionsCount() const 
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		if (!mMaxTransactionNr && !mMinTransactionNr) return 0;
		return mMaxTransactionNr - mMinTransactionNr + 1; 
	}
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
