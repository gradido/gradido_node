#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "ControllerBase.h"
#include <vector>
#include <map>

#include "Poco/Path.h"
#include "Poco/SharedPtr.h"

#include "../model/files/BlockIndex.h"
#include "../task/CPUTask.h"



namespace controller {

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

	class BlockIndex : public ControllerBase, public model::files::IBlockIndexReceiver
	{
		friend model::files::BlockIndex;
	public:
		BlockIndex(Poco::Path groupFolderPath, Poco::UInt32 blockNr);
		~BlockIndex();

		//! \brief loading block index from file (or at least try to load)
		bool loadFromFile();

		//! \brief write block index into files
		bool writeIntoFile();
		std::unique_ptr<model::files::BlockIndex> serialize();

		bool addIndicesForTransaction(Poco::SharedPtr<model::NodeTransactionEntry> transactionEntry);
		bool addIndicesForTransaction(const std::string& coinGroupId, uint16_t year, uint8_t month, uint64_t transactionNr, int32_t fileCursor, const std::vector<uint32_t>& addressIndices);
		//! implement from model::files::IBlockIndexReceiver, called by loading block index from file
		bool addIndicesForTransaction(const std::string& coinGroupId, uint16_t year, uint8_t month, uint64_t transactionNr, int32_t fileCursor, const uint32_t* addressIndices, uint8_t addressIndiceCount);

		//! \brief add transactionNr - fileCursor pair to map if not already exist
		//! \return false if transactionNr exist, else return true
		bool addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor);

		//! \brief find transaction nrs for address index in specific month and year
		//! \return empty vector in case nothing found
		std::vector<uint64_t> findTransactionsForAddressMonthYear(uint32_t addressIndex, uint16_t year, uint8_t month);

		//! \brief find transaction nrs for address index
		//! \param coinColor ignore if value is zero
		//! \return empty vector in case nothing found
		//! TODO: profile and if to slow on big data amounts, update 
		std::vector<uint64_t> findTransactionsForAddress(uint32_t addressIndex, const std::string& coinGroupId = "");

		//! \brief search from highest year and month to lowest, return youngest transaction which is fitting the search criteria
		uint64_t findLastTransactionForAddress(uint32_t addressIndex, const std::string& coinGroupId = "");
		uint64_t findFirstTransactionForAddress(uint32_t addressIndex, const std::string& coinGroupId = "");

		//! \brief find transaction nrs from specific month and year
		//! \return empty shared ptr if nothing found
		Poco::SharedPtr<std::vector<uint64_t>> findTransactionsForMonthYear(uint16_t year, uint8_t month);

		//! \param fileCursor reference to be filled with fileCursor
		//! \return true if transaction nr was found and fileCursor was set, else return false
		bool getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor);
		bool hasTransactionNr(uint64_t transactionNr) { Poco::Mutex::ScopedLock lock(mSlowWorkingMutex); return transactionNr >= mMinTransactionNr && transactionNr <= mMaxTransactionNr; }

		inline uint64_t getMaxTransactionNr() { Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);  return mMaxTransactionNr; }
		inline uint64_t getMinTransactionNr() { Poco::Mutex::ScopedLock lock(mSlowWorkingMutex); return mMinTransactionNr; }

		std::pair<uint16_t, uint8_t> getOldestYearMonth();
		std::pair<uint16_t, uint8_t> getNewestYearMonth();

		// clear maps
		void reset();

	protected:
		//! \brief called from model::files::BlockIndex while reading file
		Poco::SharedPtr<model::files::BlockIndex> mBlockIndexFile;
		uint64_t				 mMaxTransactionNr;
		uint64_t				 mMinTransactionNr;

		std::map<uint64_t, int32_t> mTransactionNrsFileCursors;
		typedef std::pair<uint64_t, int32_t> TransactionNrsFileCursorsPair;

		struct AddressIndexEntry
		{
			Poco::SharedPtr<std::vector<uint64_t>> transactionNrs;
			std::map<uint32_t, std::vector<uint32_t>> addressIndicesTransactionNrIndices;
			std::map<std::string, std::vector<uint32_t>> coinGroupIdTransactionNrIndices;
		};

		std::map<uint16_t, std::map<uint8_t, AddressIndexEntry>> mYearMonthAddressIndexEntrys;

		Poco::Mutex mSlowWorkingMutex;
		bool mDirty;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
