#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H

#include "ControllerBase.h"
#include <vector>
#include <map>

#include "Poco/Path.h"

#include "../model/files/BlockIndex.h"
#include "../model/TransactionEntry.h"

namespace controller {

	/*!
	 * @author Dario Rekowski
	 * @date 2020-02-12
	 * @brief store block index in memory for fast finding transactions
	 *
	 * map: uint64 transaction nr, uint32 file cursor
	 * map: uint32 address index, uint64 transaction nr
	 * year[month[
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

		bool addIndicesForTransaction(Poco::SharedPtr<model::TransactionEntry> transactionEntry);

		//! \brief add transactionNr - fileCursor pair to map if not already exist
		//! \return false if transactionNr exist, else return true
		bool addFileCursorForTransaction(uint64_t transactionNr, uint32_t fileCursor);

		//! \brief find transaction nrs for address index in specific month and year
		//! \return empty vector in case nothing found
		std::vector<uint64_t> findTransactionsForAddressMonthYear(uint32_t addressIndex, uint16_t year, uint8_t month);
		//! \brief find transaction nrs from specific month and year
		//! \return empty shared ptr if nothing found
		Poco::SharedPtr<std::vector<uint64_t>> findTransactionsForMonthYear(uint16_t year, uint8_t month);

		//! \param fileCursor reference to be filled with fileCursor
		//! \return true if transaction nr was found and fileCursor was set, else return false
		bool getFileCursorForTransactionNr(uint64_t transactionNr, uint32_t& fileCursor);

		inline uint64_t getMaxTransactionNr() { Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);  return mMaxTransactionNr; }
		inline uint64_t getMinTransactionNr() { Poco::Mutex::ScopedLock lock(mSlowWorkingMutex); return mMinTransactionNr; }

	protected:

		//! \brief called from model::files::BlockIndex while reading file


		model::files::BlockIndex mBlockIndexFile;
		uint64_t				 mMaxTransactionNr;
		uint64_t				 mMinTransactionNr;

		std::map<uint64_t, uint32_t> mTransactionNrsFileCursors;
		typedef std::pair<uint64_t, uint32_t> TransactionNrsFileCursorsPair;

		struct AddressIndexEntry 
		{
			Poco::SharedPtr<std::vector<uint64_t>> transactionNrs;
			std::map<uint32_t, std::vector<uint32_t>> addressIndicesTransactionNrIndices;
		};

		std::map<uint16_t, std::map<uint8_t, AddressIndexEntry>> mYearMonthAddressIndexEntrys;


		Poco::Mutex mSlowWorkingMutex;
		
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_BLOCK_INDEX_H