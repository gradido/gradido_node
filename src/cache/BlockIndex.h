#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/blockchain/TransactionsIndex.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"

#include "../blockchain/NodeTransactionEntry.h"
#include "../model/files/BlockIndex.h"
#include "../task/CPUTask.h"

#include "rapidjson/document.h"

#include <vector>
#include <map>
#include <mutex>

namespace gradido {
	namespace blockchain {
		class AbstractProvider;
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

	class BlockIndex : public gradido::blockchain::TransactionsIndex, public model::files::IBlockIndexReceiver
	{
		// friend model::files::BlockIndex;
	public:
		BlockIndex(gradido::blockchain::AbstractProvider* blockchainProvider, std::string_view groupFolderPath, uint32_t blockNr);
		~BlockIndex();

		bool init();
		void exit();
		void reset();

		//! \brief loading block index from file (or at least try to load)
		bool loadFromFile();

		//! \brief write block index into files
		std::unique_ptr<model::files::BlockIndex> serialize();
		inline rapidjson::Value serializeToJson(rapidjson::Document::AllocatorType& alloc) const;
		//! \brief
		//! \return true if there was something to write into file, after writing it to file
		bool writeIntoFile();

		bool addIndicesForTransaction(
			std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry,
			IMutableDictionary<memory::ConstBlockPtr>& publicKeyDictionary
		);

		//! implement from model::files::IBlockIndexReceiver, called by loading block index from file
		bool addIndicesForTransaction(
			gradido::data::TransactionType transactionType,
			std::optional<uint32_t> coinCommunityIdIndex,
			date::year year,
			date::month month,
			uint64_t transactionNr, 
			int32_t fileCursor, 
			const uint32_t* addressIndices,
			uint16_t addressIndiceCount,
			uint8_t isBalanceChanging
		);

		//! \brief add transactionNr - fileCursor pair to map if not already exist
		//! \return false if transactionNr exist, else return true
		bool addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor);

		//! \brief search transaction nrs for search criteria in filter, ignore filter function
		//! \return transaction nrs
		inline std::vector<uint64_t> findTransactions(const gradido::blockchain::Filter& filter, const IDictionary<memory::ConstBlockPtr>& publicKeysDictionary) const;

		//! count all, ignore pagination
		inline size_t countTransactions(const gradido::blockchain::Filter& filter, const IDictionary<memory::ConstBlockPtr>& publicKeysDictionary) const;

		//! \brief find transaction nrs from specific month and year
		//! \return {0, 0} if nothing found
		inline std::pair<uint64_t, uint64_t> findTransactionsForMonthYear(date::year year, date::month month) const;

		//! \param fileCursor reference to be filled with fileCursor
		//! \return true if transaction nr was found and fileCursor was set, else return false
		bool getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor) const;
		inline bool hasTransactionNr(uint64_t transactionNr) const;

		inline uint64_t getMaxTransactionNr() const;
		inline uint64_t getMinTransactionNr() const;
		inline uint64_t getTransactionsCount() const;

		inline date::year_month getOldestYearMonth() const;
		inline date::year_month getNewestYearMonth() const;
		inline TimepointInterval filteredTimepointInterval(const gradido::blockchain::Filter& filter) const;

	protected:

		//! \brief called from model::files::BlockIndex while reading file
		std::string				 mFolderPath;
		uint32_t				 mBlockNr;
		
		std::map<uint64_t, int32_t> mTransactionNrsFileCursors;
		typedef std::pair<uint64_t, int32_t> TransactionNrsFileCursorsPair;

		mutable std::recursive_mutex mRecursiveMutex;
		bool mDirty;
	};

	rapidjson::Value BlockIndex::serializeToJson(rapidjson::Document::AllocatorType& alloc) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::serializeToJson(alloc);
	}

	std::vector<uint64_t> BlockIndex::findTransactions(
		const gradido::blockchain::Filter& filter, 
		const IDictionary<memory::ConstBlockPtr>& publicKeysDictionary
	) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::findTransactions(filter, publicKeysDictionary);
	}

	size_t BlockIndex::countTransactions(
		const gradido::blockchain::Filter& filter, 
		const IDictionary<memory::ConstBlockPtr>& publicKeysDictionary
	) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::countTransactions(filter, publicKeysDictionary);
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
		return gradido::blockchain::TransactionsIndex::getMaxTransactionNr();
	}
	uint64_t BlockIndex::getMinTransactionNr() const 
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::getMinTransactionNr();
	}

	uint64_t BlockIndex::getTransactionsCount() const 
	{ 
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::getTransactionsCount();
	}

	date::year_month BlockIndex::getOldestYearMonth() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::getOldestYearMonth();
	}
	date::year_month BlockIndex::getNewestYearMonth() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::getNewestYearMonth();
	}

	TimepointInterval BlockIndex::filteredTimepointInterval(const gradido::blockchain::Filter& filter) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::filteredTimepointInterval(filter);
	}

	std::pair<uint64_t, uint64_t> BlockIndex::findTransactionsForMonthYear(date::year year, date::month month) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		return gradido::blockchain::TransactionsIndex::findTransactionsForMonthYear(year, month);
	}
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
