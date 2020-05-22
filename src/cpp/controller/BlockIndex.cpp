#include "BlockIndex.h"

#include "Poco/Logger.h"
#include "../SingletonManager/LoggerManager.h"

namespace controller {


	BlockIndex::BlockIndex(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
		: mBlockIndexFile(groupFolderPath, blockNr), mMaxTransactionNr(0), mMinTransactionNr(0)
	{

	}

	BlockIndex::~BlockIndex()
	{
		
	}

	bool BlockIndex::loadFromFile()
	{
		mSlowWorkingMutex.lock();
		assert(!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size());
		mSlowWorkingMutex.unlock();

		return mBlockIndexFile.readFromFile(this);
	}

	bool BlockIndex::writeIntoFile()
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		assert(mYearMonthAddressIndexEntrys.size() && mTransactionNrsFileCursors.size());

		mBlockIndexFile.reset();

		for (auto itYear = mYearMonthAddressIndexEntrys.begin(); itYear != mYearMonthAddressIndexEntrys.end(); itYear++) 
		{
			mBlockIndexFile.addYearBlock(itYear->first);
			for (auto itMonth = itYear->second.begin(); itMonth != itYear->second.end(); itMonth++) 
			{
				mBlockIndexFile.addMonthBlock(itMonth->first);

				auto addressIndexEntry = itMonth->second;
				// memory for putting address indices and transactions together
				std::vector<std::vector<uint32_t>> transactionTransactionIndices;
				transactionTransactionIndices.reserve(addressIndexEntry.transactionNrs->size());
				
				for (auto itAddressIndex = addressIndexEntry.addressIndicesTransactionNrIndices.begin(); itAddressIndex != addressIndexEntry.addressIndicesTransactionNrIndices.end(); itAddressIndex++)
				{
					auto addressIndex = itAddressIndex->first;
					for (auto itTrNrIndex = itAddressIndex->second.begin(); itTrNrIndex != itAddressIndex->second.end(); itTrNrIndex++) {
						transactionTransactionIndices[*itTrNrIndex].push_back(addressIndex);
					}
				}
				// put into file object
				int index = 0;
				for (auto itTrID = addressIndexEntry.transactionNrs->begin(); itTrID != addressIndexEntry.transactionNrs->end(); itTrID++) {
					mBlockIndexFile.addDataBlock(*itTrID, transactionTransactionIndices[index]);
					index++;
				}
			}
		}
		// finally write down to file
		return mBlockIndexFile.writeToFile();
		//return true;
	}


	bool BlockIndex::addIndicesForTransaction(Poco::SharedPtr<model::TransactionEntry> transactionEntry)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		auto fileCursor = transactionEntry->getFileCursor();
		auto transactionNr = transactionEntry->getTransactionNr();

		if (transactionNr > mMaxTransactionNr) {
			mMaxTransactionNr = transactionNr;
		}
		if (!mMinTransactionNr || transactionNr < mMinTransactionNr) {
			mMinTransactionNr = transactionNr;
		}

		// transaction nr - file cursor map
		if (fileCursor >= 0) {
			addFileCursorForTransaction(transactionNr, fileCursor);
		}
		// year
		auto yearIt = mYearMonthAddressIndexEntrys.find(transactionEntry->getYear());
		if (yearIt == mYearMonthAddressIndexEntrys.end()) {
			std::map<uint8_t, AddressIndexEntry> monthAddressIndexMap;
			auto result = mYearMonthAddressIndexEntrys.insert(std::pair<uint16_t, std::map<uint8_t, AddressIndexEntry>>
				(transactionEntry->getYear(), monthAddressIndexMap));
			yearIt = result.first;
		}
		// month
		auto monthIt = yearIt->second.find(transactionEntry->getMonth());
		if (monthIt == yearIt->second.end()) {
			auto result = yearIt->second.insert(std::pair<uint8_t, AddressIndexEntry>(transactionEntry->getMonth(), AddressIndexEntry()));
			monthIt = result.first;
			monthIt->second.transactionNrs = new std::vector<uint64_t>;
		}
		auto addressIndexEntry = &monthIt->second;
		addressIndexEntry->transactionNrs->push_back(transactionNr);
		uint32_t transactionNrIndex = addressIndexEntry->transactionNrs->size() - 1;

		// address index - transactions nr map
		auto addressIndices = transactionEntry->getAddressIndices();
		auto addressIndexTrans = &addressIndexEntry->addressIndicesTransactionNrIndices;
		for (auto it = addressIndices.begin(); it != addressIndices.end(); it++) {
			auto addressIndexEntry = addressIndexTrans->find(*it);
			if (addressIndexEntry == addressIndexTrans->end()) {
				//uint32_t, std::vector<uint32_t>
				addressIndexTrans->insert(std::pair<uint32_t, std::vector<uint32_t>>(*it, transactionNrIndex));
			}
			else {
				addressIndexEntry->second.push_back(transactionNrIndex);
			}
		}
		return true;
	}

	bool BlockIndex::addFileCursorForTransaction(uint64_t transactionNr, uint32_t fileCursor)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		auto result = mTransactionNrsFileCursors.insert(TransactionNrsFileCursorsPair(transactionNr, fileCursor));
		return result.second;
	}

	std::vector<uint64_t> BlockIndex::findTransactionsForAddressMonthYear(uint32_t addressIndex, uint16_t year, uint8_t month)
	{
		std::vector<uint64_t> result;
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		auto yearIt = mYearMonthAddressIndexEntrys.find(year);
		if (yearIt == mYearMonthAddressIndexEntrys.end()) {
			return result;
		}
		auto monthIt = yearIt->second.find(month);
		if (monthIt == yearIt->second.end()) {
			return result;
		}
		AddressIndexEntry* addressIndexEntry = &monthIt->second;
		auto addressIndexIt = addressIndexEntry->addressIndicesTransactionNrIndices.find(addressIndex);
		if (addressIndexIt == addressIndexEntry->addressIndicesTransactionNrIndices.end()) {
			return result;
		}
		auto transactionNrIndices = addressIndexIt->second;
		result.reserve(transactionNrIndices.size());
		for (auto it = transactionNrIndices.begin(); it != transactionNrIndices.end(); it++) {
			result.push_back((*addressIndexEntry->transactionNrs)[*it]);
		}
		return result;
	}

	Poco::SharedPtr<std::vector<uint64_t>> BlockIndex::findTransactionsForMonthYear(uint16_t year, uint8_t month)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		auto yearIt = mYearMonthAddressIndexEntrys.find(year);
		if (yearIt == mYearMonthAddressIndexEntrys.end()) {
			return Poco::SharedPtr<std::vector<uint64_t>>();
		}
		auto monthIt = yearIt->second.find(month);
		if (monthIt == yearIt->second.end()) {
			return Poco::SharedPtr<std::vector<uint64_t>>();
		}
		return monthIt->second.transactionNrs;
	}

	bool BlockIndex::getFileCursorForTransactionNr(uint64_t transactionNr, uint32_t& fileCursor)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		auto it = mTransactionNrsFileCursors.find(transactionNr);
		if (it != mTransactionNrsFileCursors.end()) {
			fileCursor = it->second;
			return true;
		} 

		return false;
	}

}