#include "BlockIndex.h"
#include "Exceptions.h"
#include "../controller/ControllerExceptions.h"
#include "../task/HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBasedProvider.h"

#include "loguru/loguru.hpp"

using namespace gradido::blockchain;
using namespace gradido::data;

namespace cache {


	BlockIndex::BlockIndex(std::string_view groupFolderPath, uint32_t blockNr)
		: mFolderPath(groupFolderPath), mBlockNr(blockNr), mMaxTransactionNr(0), mMinTransactionNr(0),
		mDirty(false)
	{

	}

	BlockIndex::~BlockIndex()
	{
		exit();
	}

	bool BlockIndex::init()
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (loadFromFile()) {
			return true;
		}
		return false;
	}

	void BlockIndex::exit()
	{
		std::lock_guard _lock(mRecursiveMutex);
		// Todo: store at runtime like Dictionary
		writeIntoFile();
		clearIndexEntries();
		mTransactionNrsFileCursors.clear();
		mMaxTransactionNr = 0;
		mMinTransactionNr = 0;
	}

	void BlockIndex::reset()
	{
		std::lock_guard _lock(mRecursiveMutex);
		clearIndexEntries();
		mTransactionNrsFileCursors.clear();		
		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr);
		LOG_F(WARNING, "BlockIndex: %s was corrupted and must be rebuild", blockIndexFile.getFileName().data());
		blockIndexFile.reset();
		mMaxTransactionNr = 0;
		mMinTransactionNr = 0;
	}

	bool BlockIndex::loadFromFile()
	{
		std::lock_guard _lock(mRecursiveMutex);
		assert(!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size());

		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr);
		return blockIndexFile.readFromFile(this);
	}

	std::unique_ptr<model::files::BlockIndex> BlockIndex::serialize()
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntrys.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to save
			return nullptr;
		}
		
		assert(mYearMonthAddressIndexEntrys.size() && mTransactionNrsFileCursors.size());
		auto blockIndexFile = std::make_unique<model::files::BlockIndex>(mFolderPath, mBlockNr);

		std::vector<uint32_t> publicKeyIndicesTemp;
		publicKeyIndicesTemp.reserve(10);
		for (auto itYear = mYearMonthAddressIndexEntrys.begin(); itYear != mYearMonthAddressIndexEntrys.end(); itYear++)
		{
			blockIndexFile->addYearBlock(itYear->first);
			for (auto itMonth = itYear->second.begin(); itMonth != itYear->second.end(); itMonth++)
			{
				blockIndexFile->addMonthBlock(itMonth->first);
				for (const auto& blockIndexEntry : itMonth->second) {
					auto fileCursorIt = mTransactionNrsFileCursors.find(blockIndexEntry.transactionNr);
					if (fileCursorIt == mTransactionNrsFileCursors.end()) {
						throw GradidoNodeInvalidDataException("missing file cursor for transaction");
					}
					publicKeyIndicesTemp.clear();
					for (auto i = 0; i < blockIndexEntry.addressIndiceCount; i++) {
						publicKeyIndicesTemp.push_back(blockIndexEntry.addressIndices[i]);
					}
					blockIndexFile->addDataBlock(
						blockIndexEntry.transactionNr,
						fileCursorIt->second,
						blockIndexEntry.transactionType, 
						blockIndexEntry.coinCommunityIdIndex,
						publicKeyIndicesTemp
					);
				}
			}
		}
		// finally write down to file
		return std::move(blockIndexFile);
	}

	bool BlockIndex::writeIntoFile()
	{
		auto blockIndexFile = serialize();
		if (blockIndexFile) {
			blockIndexFile->writeToFile();
			return true;
		}
		return false;
	}

	bool BlockIndex::addIndicesForTransaction(
		gradido::data::TransactionType transactionType,
		uint32_t coinCommunityIdIndex,
		date::year year, 
		date::month month,
		uint64_t transactionNr, 
		int32_t fileCursor, 
		const uint32_t* addressIndices, 
		uint16_t addressIndiceCount
	)
	{
		std::lock_guard _lock(mRecursiveMutex);
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
			auto result = mYearMonthAddressIndexEntrys.insert({ year, {} });
			yearIt = result.first;
		}
		// month
		auto monthIt = yearIt->second.find(month);
		if (monthIt == yearIt->second.end()) {
			auto result = yearIt->second.insert({ month, std::list<BlockIndexEntry>() });
			monthIt = result.first;
		}
		BlockIndexEntry entry;
		entry.transactionNr = transactionNr;
		entry.coinCommunityIdIndex = coinCommunityIdIndex;
		entry.transactionType = transactionType;
		entry.addressIndiceCount = addressIndiceCount;
		if (addressIndiceCount > 256) {
			throw GradidoNodeInvalidDataException("addressIndiceCount is bigger than 256, that cannot be");
		}
		entry.addressIndices = new uint32_t[addressIndiceCount];
		memcpy(entry.addressIndices, addressIndices, sizeof(uint32_t) * addressIndiceCount);

		if (monthIt->second.size() && monthIt->second.back().transactionNr >= entry.transactionNr) {
			throw BlockIndexException("try to add new transaction to block index with same or lesser transaction nr!");
		}
		monthIt->second.push_back(entry);

		addFileCursorForTransaction(transactionNr, fileCursor);		
		return true;
	}

	bool BlockIndex::addIndicesForTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry)
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
		auto fbp = gradido::blockchain::FileBasedProvider::getInstance();
		
		uint32_t coinCommunityIndex = 0;
		if (!transactionEntry->getCoinCommunityId().empty()) {
			coinCommunityIndex = fbp->getCommunityIdIndex(transactionEntry->getCoinCommunityId());
		}
		auto& publicKeyIndices = transactionEntry->getAddressIndices();
		return addIndicesForTransaction(
			transactionEntry->getTransactionType(),
			coinCommunityIndex,
			transactionEntry->getYear(),
			transactionEntry->getMonth(),
			transactionNr,
			transactionEntry->getFileCursor(),
			publicKeyIndices.data(),
			static_cast<uint16_t>(publicKeyIndices.size())
		);
		
	}

	bool BlockIndex::addFileCursorForTransaction(uint64_t transactionNr, int32_t fileCursor)
	{
		if (fileCursor < 0) return false;
		if (!fileCursor) {
			LOG_F(1, "fileCursor is 0, transactionNr: %u", (unsigned)transactionNr);
		}
		std::lock_guard _lock(mRecursiveMutex);
		if (!fileCursor && transactionNr > mMinTransactionNr) {
			throw BlockIndexInvalidFileCursorException("file curser is invalid", fileCursor, transactionNr, mMinTransactionNr);
		}
		
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

	std::vector<uint64_t> BlockIndex::findTransactions(const gradido::blockchain::Filter& filter, const Dictionary& publicKeysDictionary) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		// prefilter
		if ((filter.minTransactionNr && filter.minTransactionNr > mMaxTransactionNr) ||
			(filter.maxTransactionNr && filter.maxTransactionNr < mMinTransactionNr)) {
			return {};
		}
		uint32_t publicKeyIndex = 0;
		if (filter.involvedPublicKey && !filter.involvedPublicKey->isEmpty()) {
			auto involvedPublicKeyCopy = filter.involvedPublicKey->copyAsString();
			if (!publicKeysDictionary.hasString(involvedPublicKeyCopy)) {
				return {};
			}
			publicKeyIndex = publicKeysDictionary.getIndexForString(involvedPublicKeyCopy);
		}
		uint32_t coinCommunityKeyIndex = 0;
		if (!filter.coinCommunityId.empty()) {
			coinCommunityKeyIndex = FileBasedProvider::getInstance()->getCommunityIdIndex(filter.coinCommunityId);
		}
		TimepointInterval interval(getOldestYearMonth(), getNewestYearMonth());
		if (!filter.timepointInterval.isEmpty()) {
			if (interval.getStartDate() < filter.timepointInterval.getStartDate()) {
				interval.setStartDate(filter.timepointInterval.getStartDate());
			}
			if (interval.getEndDate() > filter.timepointInterval.getEndDate()) {
				interval.setEndDate(std::max(interval.getStartDate(), filter.timepointInterval.getEndDate()));
			}
		} 
		std::vector<uint64_t> result;
		if (filter.pagination.size) {
			result.reserve(filter.pagination.size);
		}
		unsigned int entryCursor = 0;
		auto matchingFunction = [&](const BlockIndexEntry& blockIndexEntry) -> bool 
		{
			if (filter.transactionType != TransactionType::NONE
				&& filter.transactionType != blockIndexEntry.transactionType) {
				return true;
			}
			if (coinCommunityKeyIndex
				&& coinCommunityKeyIndex != blockIndexEntry.coinCommunityIdIndex) {
				return true;
			}
			if (filter.minTransactionNr
				&& filter.minTransactionNr > blockIndexEntry.transactionNr) {
				return true;
			}
			if (filter.maxTransactionNr
				&& filter.maxTransactionNr < blockIndexEntry.transactionNr) {
				return true;
			}
			if (publicKeyIndex) {
				bool found = false;
				for (int iPublicKeyIndices = 0; iPublicKeyIndices < blockIndexEntry.addressIndiceCount; iPublicKeyIndices++) {
					if (publicKeyIndex == blockIndexEntry.addressIndices[iPublicKeyIndices]) {
						found = true;
						break;
					}
				}
				if (!found) {
					return true;
				}
			}
			if (entryCursor < filter.pagination.skipEntriesCount()) {
				return true;
			}
			result.push_back(blockIndexEntry.transactionNr);
			if (!filter.pagination.hasCapacityLeft(result.size())) {
				return false; //break
			}
			entryCursor++;
			return true;
		};
		/*
		LOG_F(1, "startDate: %s, endDate: %s",
			DataTypeConverter::timePointToString(interval.getStartDate()).data(),
			DataTypeConverter::timePointToString(interval.getEndDate()).data()
		);
		*/
		bool revert = filter.searchDirection == SearchDirection::DESC;
		auto intervalIt = revert ? std::prev(interval.end()) : interval.begin();
		auto intervalEnd = revert ? std::prev(interval.begin()) : interval.end();		
		
		while (intervalIt != intervalEnd) 
		{
			auto yearIt = mYearMonthAddressIndexEntrys.find(intervalIt->year());
			assert(yearIt != mYearMonthAddressIndexEntrys.end());
			auto monthIt = yearIt->second.find(intervalIt->month());
			assert(monthIt != yearIt->second.end());			

			if (revert) {
				for (auto it = monthIt->second.rbegin(); it != monthIt->second.rend(); ++it) {
					if (!matchingFunction(*it)) break;
				}
			}
			else {
				for (auto it = monthIt->second.begin(); it != monthIt->second.end(); ++it) {
					if (!matchingFunction(*it)) break;
				}
			}
			if (revert) {
				intervalIt--;
			} else {
				intervalIt++;
			}
			// if we already have enough results for the caller, let's stop here
			if (!filter.pagination.hasCapacityLeft(result.size())) {
				break;
			}
			
		}
		// TODO: check if neccessary, shouldn't be
		if (revert) {
			// sort in descending order
			std::sort(result.begin(), result.end(), std::greater<>());
		} else {
			// sort in ascending order
			std::sort(result.begin(), result.end());
		}
		return result;
	}
	// TODO: reduce double code
	size_t BlockIndex::countTransactions(const gradido::blockchain::Filter& filter, const Dictionary& publicKeysDictionary) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		// prefilter
		if ((filter.minTransactionNr && filter.minTransactionNr > mMaxTransactionNr) ||
			(filter.maxTransactionNr && filter.maxTransactionNr < mMinTransactionNr)) {
			return 0;
		}
		uint32_t publicKeyIndex = 0;
		if (filter.involvedPublicKey && !filter.involvedPublicKey->isEmpty()) {
			auto involvedPublicKeyCopy = filter.involvedPublicKey->copyAsString();
			if (publicKeysDictionary.hasString(involvedPublicKeyCopy)) {
				publicKeyIndex = publicKeysDictionary.getIndexForString(involvedPublicKeyCopy);
			}
		}
		uint32_t coinCommunityKeyIndex = 0;
		if (!filter.coinCommunityId.empty()) {
			coinCommunityKeyIndex = FileBasedProvider::getInstance()->getCommunityIdIndex(filter.coinCommunityId);
		}
		TimepointInterval interval(getOldestYearMonth(), getNewestYearMonth());
		if (!filter.timepointInterval.isEmpty()) {
			if (interval.getStartDate() < filter.timepointInterval.getStartDate()) {
				interval.setStartDate(filter.timepointInterval.getStartDate());
			}
			if (interval.getEndDate() > filter.timepointInterval.getEndDate()) {
				interval.setEndDate(filter.timepointInterval.getEndDate());
			}
		}
		bool revert = filter.searchDirection == SearchDirection::DESC;
		auto intervalIt = revert ? std::prev(interval.end()) : interval.begin();
		auto intervalEnd = revert ? std::prev(interval.begin()) : interval.end();
		size_t result = 0;
		unsigned int entryCursor = 0;
		while (intervalIt != intervalEnd)
		{
			auto yearIt = mYearMonthAddressIndexEntrys.find(intervalIt->year());
			// assert(yearIt != mYearMonthAddressIndexEntrys.end());
			if (yearIt == mYearMonthAddressIndexEntrys.end()) {
				LOG_F(ERROR, "yearIt goes beyond array borders, tried to find: %d", (int)intervalIt->year());
				break;
			}
			auto monthIt = yearIt->second.find(intervalIt->month());
			//assert(monthIt != yearIt->second.end());
			if (monthIt == yearIt->second.end()) {
				LOG_F(ERROR, "monthIt goes beyond array borders, tried to find: %u", (unsigned)intervalIt->month());
				break;
			}
			assert(monthIt != yearIt->second.end());

			for (const auto& blockIndexEntry : monthIt->second) {
				if (filter.transactionType != TransactionType::NONE
					&& filter.transactionType != blockIndexEntry.transactionType) {
					continue;
				}
				if (coinCommunityKeyIndex
					&& coinCommunityKeyIndex != blockIndexEntry.coinCommunityIdIndex) {
					continue;
				}
				if (filter.minTransactionNr
					&& filter.minTransactionNr > blockIndexEntry.transactionNr) {
					continue;
				}
				if (filter.maxTransactionNr
					&& filter.maxTransactionNr < blockIndexEntry.transactionNr) {
					continue;
				}
				if (publicKeyIndex) {
					bool found = false;
					for (int iPublicKeyIndices = 0; iPublicKeyIndices < blockIndexEntry.addressIndiceCount; iPublicKeyIndices++) {
						if (publicKeyIndex == blockIndexEntry.addressIndices[iPublicKeyIndices]) {
							found = true;
							break;
						}
					}
					if (!found) {
						continue;
					}
				}
				result++;
				entryCursor++;
			}
			if (revert) {
				intervalIt--;
			}
			else {
				intervalIt++;
			}
		}
		return result;
	}

	std::pair<uint64_t, uint64_t> BlockIndex::findTransactionsForMonthYear(date::year year, date::month month) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		auto yearIt = mYearMonthAddressIndexEntrys.find(year);
		if (yearIt == mYearMonthAddressIndexEntrys.end()) {
			return { 0, 0 };
		}
		auto monthIt = yearIt->second.find(month);
		if (monthIt == yearIt->second.end()) {
			return { 0, 0 };
		}
		return { monthIt->second.front().transactionNr, monthIt->second.back().transactionNr };
	}

	bool BlockIndex::getFileCursorForTransactionNr(uint64_t transactionNr, int32_t& fileCursor) const
	{
		std::lock_guard _lock(mRecursiveMutex);

		auto it = mTransactionNrsFileCursors.find(transactionNr);
		if (it != mTransactionNrsFileCursors.end()) {
			// by transaction nr 1 file cursor starts with 0
			if(!it->second && transactionNr > 1) {
				auto lastTransaction = mTransactionNrsFileCursors.end();
				lastTransaction--;
				LOG_F(INFO, "file cursor for transaction: %" PRIu64 " is : %d, last transaction: %" PRIu64 ", cursor: %d",
					transactionNr,
					it->second,
					lastTransaction->first,
					lastTransaction->second
				);
				long timeout = 100;
			    do {
					it = mTransactionNrsFileCursors.find(transactionNr);
					if (it == mTransactionNrsFileCursors.end()) {
						return false;
					}
					if(it->second) break;
					mRecursiveMutex.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
					mRecursiveMutex.lock();
					timeout--;					
				} while(!it->second && timeout > 0);
				LOG_F(INFO, "timeout: %ld, file cursor: %d", timeout, it->second);
			}
			fileCursor = it->second;
			return true;
		} 

		return false;
	}

	date::year_month BlockIndex::getOldestYearMonth() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntrys.size()) {
			return { date::year(0), date::month(0) };
		}
		auto firstEntry = mYearMonthAddressIndexEntrys.begin();
		assert(firstEntry->second.size());
		return { firstEntry->first, firstEntry->second.begin()->first };
	}
	date::year_month BlockIndex::getNewestYearMonth() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntrys.size()) {
			return { date::year(0), date::month(0) };
		}
		auto lastEntry = std::prev(mYearMonthAddressIndexEntrys.end());
		assert(lastEntry->second.size());
		auto lastMonthEntry = std::prev(lastEntry->second.end());
		return { lastEntry->first, lastMonthEntry->first };
	}

	void BlockIndex::clearIndexEntries()
	{
		std::lock_guard _lock(mRecursiveMutex);
		for (auto& yearBlock : mYearMonthAddressIndexEntrys) {
			for (auto& monthBlock : yearBlock.second) {
				for (auto& blockIndexEntry : monthBlock.second) {
					delete blockIndexEntry.addressIndices;
				}
			}
		}
		mYearMonthAddressIndexEntrys.clear();
	}

}