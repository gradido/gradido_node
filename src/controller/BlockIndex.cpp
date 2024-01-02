#include "BlockIndex.h"
#include "ControllerExceptions.h"

#include "Poco/Logger.h"
#include "../SingletonManager/LoggerManager.h"

#include "../model/NodeTransactionEntry.h"

#include "../task/HddWriteBufferTask.h"

#include "../ServerGlobals.h"

namespace controller {


	BlockIndex::BlockIndex(Poco::Path groupFolderPath, Poco::UInt32 blockNr)
		: mBlockIndexFile(new model::files::BlockIndex(groupFolderPath, blockNr)), mMaxTransactionNr(0), mMinTransactionNr(0),
		mDirty(false)
	{

	}

	BlockIndex::~BlockIndex()
	{
		writeIntoFile();
	}

	bool BlockIndex::loadFromFile()
	{
		mSlowWorkingMutex.lock();
		assert(!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size());
		mSlowWorkingMutex.unlock();

		return mBlockIndexFile->readFromFile(this);
	}

	bool BlockIndex::writeIntoFile()
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		if (!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to save
			return true;
		}
		// TODO: Change, this function is called in deconstructor and it isn't a good idea to assert while in deconstructor
		assert(mYearMonthAddressIndexEntrys.size() && mTransactionNrsFileCursors.size());

		mBlockIndexFile->reset();

		for (auto itYear = mYearMonthAddressIndexEntrys.begin(); itYear != mYearMonthAddressIndexEntrys.end(); itYear++) 
		{
			mBlockIndexFile->addYearBlock(itYear->first);
			for (auto itMonth = itYear->second.begin(); itMonth != itYear->second.end(); itMonth++) 
			{
				mBlockIndexFile->addMonthBlock(itMonth->first);

				auto addressIndexEntry = itMonth->second;
				// memory for putting address indices and transactions together
				std::vector<std::vector<uint32_t>> transactionTransactionIndices;
				transactionTransactionIndices.reserve(addressIndexEntry.transactionNrs->size());
				for (int i = 0; i < addressIndexEntry.transactionNrs->size(); i++) {
					transactionTransactionIndices.push_back(std::vector<uint32_t>());
				}
				//std::vector<uint32_t>* transactionTransactionIndices = new std::vector<uint32_t>[addressIndexEntry.transactionNrs->size()];
				//transactionTransactionIndices.reserve(addressIndexEntry.transactionNrs->size());
				
				for (auto itAddressIndex = addressIndexEntry.addressIndicesTransactionNrIndices.begin(); itAddressIndex != addressIndexEntry.addressIndicesTransactionNrIndices.end(); itAddressIndex++)
				{
					auto addressIndex = itAddressIndex->first;
					for (auto itTrNrIndex = itAddressIndex->second.begin(); itTrNrIndex != itAddressIndex->second.end(); itTrNrIndex++) {
						transactionTransactionIndices[*itTrNrIndex].push_back(addressIndex);
					}
				}
				// reverse coin group id				
				std::vector<std::string> coinGroupIdsForTransactionNrs;
				coinGroupIdsForTransactionNrs.reserve(addressIndexEntry.transactionNrs->size());
				auto coinGroupTranInd = &addressIndexEntry.coinGroupIdTransactionNrIndices;
				for (auto itCoinColorInd = coinGroupTranInd->begin(); itCoinColorInd != coinGroupTranInd->end(); itCoinColorInd++) {
					// first = coin color
					// second = vector with transaction nr indices
					for (auto itTranInd = itCoinColorInd->second.begin(); itTranInd != itCoinColorInd->second.end(); itTranInd++) {
						// TODO: assert was triggered, check why
						//assert(*itTranInd == coinColorsForTransactionNrs.size());
						coinGroupIdsForTransactionNrs.push_back(itCoinColorInd->first);
						//coinColorsForTransactionNrs[*itTranInd] = itCoinColorInd->first;
					}
				}

				// put into file object
				int index = 0;
				for (auto itTrID = addressIndexEntry.transactionNrs->begin(); itTrID != addressIndexEntry.transactionNrs->end(); itTrID++) {
					Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
					mBlockIndexFile->addDataBlock(*itTrID, mTransactionNrsFileCursors[*itTrID], coinGroupIdsForTransactionNrs[index], transactionTransactionIndices[index]);
					index++;
				}
			}
		}
		// finally write down to file
		return mBlockIndexFile->writeToFile();
		//return true;
	}
	std::unique_ptr<model::files::BlockIndex> BlockIndex::serialize()
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		if (!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to save
			return nullptr;
		}
		
		assert(mYearMonthAddressIndexEntrys.size() && mTransactionNrsFileCursors.size());
		auto blockIndexFile = std::make_unique<model::files::BlockIndex>(mBlockIndexFile->getFileName());

		for (auto itYear = mYearMonthAddressIndexEntrys.begin(); itYear != mYearMonthAddressIndexEntrys.end(); itYear++)
		{
			blockIndexFile->addYearBlock(itYear->first);
			for (auto itMonth = itYear->second.begin(); itMonth != itYear->second.end(); itMonth++)
			{
				blockIndexFile->addMonthBlock(itMonth->first);

				auto addressIndexEntry = itMonth->second;
				// memory for putting address indices and transactions together
				std::vector<std::vector<uint32_t>> transactionTransactionIndices;
				transactionTransactionIndices.reserve(addressIndexEntry.transactionNrs->size());
				for (int i = 0; i < addressIndexEntry.transactionNrs->size(); i++) {
					transactionTransactionIndices.push_back(std::vector<uint32_t>());
				}
				//std::vector<uint32_t>* transactionTransactionIndices = new std::vector<uint32_t>[addressIndexEntry.transactionNrs->size()];
				//transactionTransactionIndices.reserve(addressIndexEntry.transactionNrs->size());

				for (auto itAddressIndex = addressIndexEntry.addressIndicesTransactionNrIndices.begin(); itAddressIndex != addressIndexEntry.addressIndicesTransactionNrIndices.end(); itAddressIndex++)
				{
					auto addressIndex = itAddressIndex->first;
					for (auto itTrNrIndex = itAddressIndex->second.begin(); itTrNrIndex != itAddressIndex->second.end(); itTrNrIndex++) {
						transactionTransactionIndices[*itTrNrIndex].push_back(addressIndex);
					}
				}
				// reverse coin group id				
				std::vector<std::string> coinGroupIdsForTransactionNrs;
				coinGroupIdsForTransactionNrs.reserve(addressIndexEntry.transactionNrs->size());
				auto coinGroupTranInd = &addressIndexEntry.coinGroupIdTransactionNrIndices;
				for (auto itCoinColorInd = coinGroupTranInd->begin(); itCoinColorInd != coinGroupTranInd->end(); itCoinColorInd++) {
					// first = coin color
					// second = vector with transaction nr indices
					for (auto itTranInd = itCoinColorInd->second.begin(); itTranInd != itCoinColorInd->second.end(); itTranInd++) {
						// TODO: assert was triggered, check why
						//assert(*itTranInd == coinColorsForTransactionNrs.size());
						coinGroupIdsForTransactionNrs.push_back(itCoinColorInd->first);
						//coinColorsForTransactionNrs[*itTranInd] = itCoinColorInd->first;
					}
				}

				// put into file object
				int index = 0;
				for (auto itTrID = addressIndexEntry.transactionNrs->begin(); itTrID != addressIndexEntry.transactionNrs->end(); itTrID++) {
					Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
					mBlockIndexFile->addDataBlock(*itTrID, mTransactionNrsFileCursors[*itTrID], coinGroupIdsForTransactionNrs[index], transactionTransactionIndices[index]);
					index++;
				}
			}
		}
		// finally write down to file
		return std::move(blockIndexFile);
	}
	
	bool BlockIndex::addIndicesForTransaction(const std::string& coinGroupId, uint16_t year, uint8_t month, uint64_t transactionNr, int32_t fileCursor, const std::vector<uint32_t>& addressIndices)
	{
		return addIndicesForTransaction(coinGroupId, year, month, transactionNr, fileCursor, addressIndices.data(), addressIndices.size());
	}

	bool BlockIndex::addIndicesForTransaction(const std::string& coinGroupId, uint16_t year, uint8_t month, uint64_t transactionNr, int32_t fileCursor, const uint32_t* addressIndices, uint8_t addressIndiceCount)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		mDirty = true;
		if (transactionNr > mMaxTransactionNr) {
			mMaxTransactionNr = transactionNr;
		}
		if (!mMinTransactionNr || transactionNr < mMinTransactionNr) {
			mMinTransactionNr = transactionNr;
		}

		// year
		auto yearIt = mYearMonthAddressIndexEntrys.find(year);
		if (yearIt == mYearMonthAddressIndexEntrys.end()) {
			std::map<uint8_t, AddressIndexEntry> monthAddressIndexMap;
			auto result = mYearMonthAddressIndexEntrys.insert(std::pair<uint16_t, std::map<uint8_t, AddressIndexEntry>>
				(year, monthAddressIndexMap));
			yearIt = result.first;
		}
		// month
		auto monthIt = yearIt->second.find(month);
		if (monthIt == yearIt->second.end()) {
			auto result = yearIt->second.insert(std::pair<uint8_t, AddressIndexEntry>(month, AddressIndexEntry()));
			monthIt = result.first;
			monthIt->second.transactionNrs = new std::vector<uint64_t>;
		}
		auto addressIndexEntry = &monthIt->second;
		// TODO: doubletten Check
		addressIndexEntry->transactionNrs->push_back(transactionNr);
		uint32_t transactionNrIndex = addressIndexEntry->transactionNrs->size() - 1;

		// address index - transactions nr map
		auto addressIndexTrans = &addressIndexEntry->addressIndicesTransactionNrIndices;
		for (int i = 0; i < addressIndiceCount; i++) 
		{
			auto value = addressIndices[i];
			auto addressIndexEntry = addressIndexTrans->find(value);
			if (addressIndexEntry == addressIndexTrans->end()) {
				//uint32_t, std::vector<uint32_t>
				addressIndexTrans->insert({ value, { transactionNrIndex } });
			}
			else {
				addressIndexEntry->second.push_back(transactionNrIndex);
			}
		}
		// coin color 
		auto coinGroupIdIndexTrans = &addressIndexEntry->coinGroupIdTransactionNrIndices;
		auto coinColorIt = coinGroupIdIndexTrans->find(coinGroupId);
		if (coinColorIt == coinGroupIdIndexTrans->end()) {
			coinGroupIdIndexTrans->insert({ coinGroupId, {transactionNrIndex} });
		}
		else {
			coinColorIt->second.push_back(transactionNrIndex);
		}
		addFileCursorForTransaction(transactionNr, fileCursor);
		
		return true;
	}

	bool BlockIndex::addIndicesForTransaction(Poco::SharedPtr<model::NodeTransactionEntry> transactionEntry)
	{
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

		return addIndicesForTransaction(
			transactionEntry->getCoinGroupId(),
			transactionEntry->getYear(),
			transactionEntry->getMonth(),
			transactionNr,
			transactionEntry->getFileCursor(),
			transactionEntry->getAddressIndices()
		);
		
	}

	bool BlockIndex::addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor)
	{
		if (fileCursor < 0) return false;
		if (!fileCursor) {
			LoggerManager::getInstance()->mErrorLogging.debug("[BlockIndex::addFileCursorForTransaction] fileCursor is 0, transactionNr: %u", (unsigned)transactionNr);
		}
		if (!fileCursor && transactionNr > mMinTransactionNr) {
			throw BlockIndexInvalidFileCursorException("file curser is invalid", fileCursor, transactionNr, mMinTransactionNr);
		}
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		mDirty = true;

		// check first if map entry already exist
		// I don't know why, but with small values for caching the block index map contains entries for transactions nrs
		// with 0 for fileCursor, but this is the only place where insert is called and fileCursor is only allowed to be 0 for
		// the first transaction per block!
		auto it = mTransactionNrsFileCursors.find(transactionNr);
		if (it == mTransactionNrsFileCursors.end()) {
			auto result = mTransactionNrsFileCursors.insert(TransactionNrsFileCursorsPair(transactionNr, fileCursor));
			if (!result.second) {
				throw BlockIndexException("error by inserting file cursor");
			}
			return result.second;
		}
		else {
			if (!it->second) {
				it->second = fileCursor;
				return true;
			}
		}
		return false;
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

	
	std::vector<uint64_t> BlockIndex::findTransactionsForAddress(uint32_t addressIndex, const std::string& coinGroupId /*= ""*/)
	{
		std::vector<uint64_t> result;
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		for (auto yearIt = mYearMonthAddressIndexEntrys.begin(); yearIt != mYearMonthAddressIndexEntrys.end(); yearIt++) {
			for (auto monthIt = yearIt->second.begin(); monthIt != yearIt->second.end(); monthIt++) {
				AddressIndexEntry* addressIndexEntry = &monthIt->second;
				auto addressIndexIt = addressIndexEntry->addressIndicesTransactionNrIndices.find(addressIndex);
				auto traIndForCoinGroupId = addressIndexEntry->coinGroupIdTransactionNrIndices.find(coinGroupId);
				if (coinGroupId.size() && traIndForCoinGroupId == addressIndexEntry->coinGroupIdTransactionNrIndices.end()) {
					continue;
				}
				if (addressIndexIt != addressIndexEntry->addressIndicesTransactionNrIndices.end()) {
					auto transactionNrIndices = addressIndexIt->second;
					for (auto it = transactionNrIndices.begin(); it != transactionNrIndices.end(); it++) {
						bool skip = false;
						if (coinGroupId.size()) {
							skip = true;
							for (auto itTransactionIndex = traIndForCoinGroupId->second.begin(); itTransactionIndex != traIndForCoinGroupId->second.end(); itTransactionIndex++) {
								if (*it == *itTransactionIndex) {
									skip = false;
									break;
								}
							}
						}
						if (!skip) {
							result.push_back((*addressIndexEntry->transactionNrs)[*it]);
						}
					}				
				}
			}
		}
		return result;
	}

	uint64_t BlockIndex::findLastTransactionForAddress(uint32_t addressIndex, const std::string& coinGroupId/* = ""*/)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		std::vector<uint64_t> result;
		for (auto yearIt = mYearMonthAddressIndexEntrys.rbegin(); yearIt != mYearMonthAddressIndexEntrys.rend(); ++yearIt) {
			for (auto monthIt = yearIt->second.rbegin(); monthIt != yearIt->second.rend(); ++monthIt) {
				AddressIndexEntry* addressIndexEntry = &monthIt->second;
				auto addressIndexIt = addressIndexEntry->addressIndicesTransactionNrIndices.find(addressIndex);
				auto traIndForCoinColor = addressIndexEntry->coinGroupIdTransactionNrIndices.find(coinGroupId);
				if (coinGroupId.size() && traIndForCoinColor == addressIndexEntry->coinGroupIdTransactionNrIndices.end()) {
					continue;
				}
				if (addressIndexIt != addressIndexEntry->addressIndicesTransactionNrIndices.end()) {
					auto transactionNrIndices = addressIndexIt->second;
					for (auto it = transactionNrIndices.begin(); it != transactionNrIndices.end(); it++) {
						bool skip = false;
						if (coinGroupId.size()) {
							skip = true;
							for (auto itTransactionIndex = traIndForCoinColor->second.begin(); itTransactionIndex != traIndForCoinColor->second.end(); itTransactionIndex++) {
								if (*it == *itTransactionIndex) {
									skip = false;
									break;
								}
							}
						}
						if (!skip) {
							result.push_back((*addressIndexEntry->transactionNrs)[*it]);
						}
					}
				}
				if (result.size() > 0) {
					std::sort(result.begin(), result.end());
					return result.back();
				}
			}
		}
		return 0;
	}

	uint64_t BlockIndex::findFirstTransactionForAddress(uint32_t addressIndex, const std::string& coinGroupId/* = ""*/)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		std::vector<uint64_t> result;
		for (auto yearIt = mYearMonthAddressIndexEntrys.begin(); yearIt != mYearMonthAddressIndexEntrys.end(); ++yearIt) {
			for (auto monthIt = yearIt->second.begin(); monthIt != yearIt->second.end(); ++monthIt) {
				AddressIndexEntry* addressIndexEntry = &monthIt->second;
				auto addressIndexIt = addressIndexEntry->addressIndicesTransactionNrIndices.find(addressIndex);
				auto traIndForCoinGroupId = addressIndexEntry->coinGroupIdTransactionNrIndices.find(coinGroupId);
				if (coinGroupId.size() && traIndForCoinGroupId == addressIndexEntry->coinGroupIdTransactionNrIndices.end()) {
					continue;
				}
				if (addressIndexIt != addressIndexEntry->addressIndicesTransactionNrIndices.end()) {
					auto transactionNrIndices = addressIndexIt->second;
					for (auto it = transactionNrIndices.begin(); it != transactionNrIndices.end(); it++) {
						bool skip = false;
						if (coinGroupId.size()) {
							skip = true;
							for (auto itTransactionIndex = traIndForCoinGroupId->second.begin(); itTransactionIndex != traIndForCoinGroupId->second.end(); itTransactionIndex++) {
								if (*it == *itTransactionIndex) {
									skip = false;
									break;
								}
							}
						}
						if (!skip) {
							result.push_back((*addressIndexEntry->transactionNrs)[*it]);
						}
					}
				}
				if (result.size() > 0) {
					std::sort(result.begin(), result.end());
					return result.front();
				}
			}
		}
		return 0;
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

	bool BlockIndex::getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor)
	{
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);

		auto it = mTransactionNrsFileCursors.find(transactionNr);
		if (it != mTransactionNrsFileCursors.end()) {
			// by transaction nr 1 file cursor starts with 0
			if(!it->second && transactionNr > 1) {
				auto lastTransaction = mTransactionNrsFileCursors.end();
				lastTransaction--;
				std::clog << "file cursor for transaction: " << transactionNr 
				          << " is : " << it->second 
						  << ", last transaction: " << lastTransaction->first
						  << ", cursor: " << lastTransaction->second
						  << std::endl;
				long timeout = 100;
			    do {
					it = mTransactionNrsFileCursors.find(transactionNr);
					if (it == mTransactionNrsFileCursors.end()) {
						return false;
					}
					if(it->second) break;
					mSlowWorkingMutex.unlock();
					Poco::Thread::sleep(20);
					mSlowWorkingMutex.lock();
					timeout--;					
				} while(!it->second && timeout > 0);
				std::clog << "timeout: " << timeout 
						  << ", file cursor: " << it->second
						  << std::endl;
			}
			fileCursor = it->second;
			return true;
		} 

		return false;
	}

	std::pair<uint16_t, uint8_t> BlockIndex::getOldestYearMonth()
	{
		if (!mYearMonthAddressIndexEntrys.size()) {
			return { 0,0 };
		}
		auto firstEntry = mYearMonthAddressIndexEntrys.begin();
		assert(firstEntry->second.size());
		return { firstEntry->first, firstEntry->second.begin()->first };
	}
	std::pair<uint16_t, uint8_t> BlockIndex::getNewestYearMonth()
	{
		if (!mYearMonthAddressIndexEntrys.size()) {
			return { 0,0 };
		}
		auto lastEntry = std::prev(mYearMonthAddressIndexEntrys.end());
		assert(lastEntry->second.size());
		auto lastMonthEntry = std::prev(lastEntry->second.end());
		return { lastEntry->first, lastMonthEntry->first };
	}

	void BlockIndex::reset()
	{
		std::clog << "BlockIndex::reset called" << std::endl;
		Poco::Mutex::ScopedLock lock(mSlowWorkingMutex);
		mTransactionNrsFileCursors.clear();
		mYearMonthAddressIndexEntrys.clear();
		mBlockIndexFile.reset();
		mMaxTransactionNr = 0;
		mMinTransactionNr = 0;
	}	

}