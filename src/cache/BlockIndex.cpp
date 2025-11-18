#include "BlockIndex.h"
#include "Exceptions.h"
#include "../controller/ControllerExceptions.h"
#include "../task/HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBasedProvider.h"
#include "gradido_blockchain/blockchain/RangeUtils.h"
#include "gradido_blockchain/serialization/toJson.h"

#include "loguru/loguru.hpp"

using namespace gradido::blockchain;
using namespace gradido::data;
using namespace rapidjson;

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
		assert(!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size());

		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr);
		return blockIndexFile.readFromFile(this);
	}

	std::unique_ptr<model::files::BlockIndex> BlockIndex::serialize()
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to save
			return nullptr;
		}
		
		assert(mYearMonthAddressIndexEntries.size() && mTransactionNrsFileCursors.size());
		auto blockIndexFile = std::make_unique<model::files::BlockIndex>(mFolderPath, mBlockNr);

		std::vector<uint32_t> publicKeyIndicesTemp;
		publicKeyIndicesTemp.reserve(10);
		for (auto itYear = mYearMonthAddressIndexEntries.begin(); itYear != mYearMonthAddressIndexEntries.end(); itYear++)
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

	Value BlockIndex::serializeToJson(Document::AllocatorType& alloc) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to show
			return Value(kNullType);
		}
		Value rootJson(kObjectType);
		for (auto itYear = mYearMonthAddressIndexEntries.begin(); itYear != mYearMonthAddressIndexEntries.end(); itYear++)
		{
			Value yearEntry(kObjectType);
			for (auto itMonth = itYear->second.begin(); itMonth != itYear->second.end(); itMonth++)
			{
				Value monthEntry(kArrayType);
				for (const auto& blockIndexEntry : itMonth->second)
				{
					Value entry(kObjectType);
					entry.AddMember("transactionNr", blockIndexEntry.transactionNr, alloc);
					entry.AddMember("transactionType", serialization::toJson(blockIndexEntry.transactionType, alloc), alloc);
					if (blockIndexEntry.coinCommunityIdIndex) {
						entry.AddMember("coinCommunityIdIndex", blockIndexEntry.coinCommunityIdIndex, alloc);
					}
					if (blockIndexEntry.addressIndiceCount) {
						Value addressIndices(kArrayType);
						for (int i = 0; i < blockIndexEntry.addressIndiceCount; i++) {
							addressIndices.PushBack(blockIndexEntry.addressIndices[i], alloc);
						}
						entry.AddMember("addressIndices", addressIndices, alloc);
					}
					auto fileCursorIt = mTransactionNrsFileCursors.find(blockIndexEntry.transactionNr);
					if (fileCursorIt != mTransactionNrsFileCursors.end()) {
						entry.AddMember("fileCursor", fileCursorIt->second, alloc);
					}
					monthEntry.PushBack(entry, alloc);
				}
				yearEntry.AddMember(serialization::toJson(itMonth->first, alloc), monthEntry, alloc);
			}
			rootJson.AddMember(serialization::toJson(itYear->first, alloc), yearEntry, alloc);
		}
		return rootJson;
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
		auto yearIt = mYearMonthAddressIndexEntries.find(year);
		if (yearIt == mYearMonthAddressIndexEntries.end()) {
			auto result = mYearMonthAddressIndexEntries.insert({ year, {} });
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
			if (publicKeysDictionary.hasString(involvedPublicKeyCopy)) {
				publicKeyIndex = publicKeysDictionary.getIndexForString(involvedPublicKeyCopy);
			} else {
				// if public key not exist, no transaction can match
				return {};
			}
		}

		std::vector<uint64_t> result;
		if (filter.pagination.size) {
			result.reserve(filter.pagination.size);
		}

		auto interval = filteredTimepointInterval(filter);
		int paginationCursor = 0;
		iterateRangeInOrder(interval.begin(), interval.end(), filter.searchDirection,
			[&](const date::year_month& timepoint) -> bool
			{
				// if for a year/month combination no entries exist, return true, so continue the loop
				auto yearIt = mYearMonthAddressIndexEntries.find(timepoint.year());
				if (yearIt == mYearMonthAddressIndexEntries.end()) {
					return true;
				}
				auto monthIt = yearIt->second.find(timepoint.month());
				if (monthIt == yearIt->second.end()) {
					return true;
				}
				iterateRangeInOrder(
					monthIt->second.begin(),
					monthIt->second.end(),
					filter.searchDirection,
					[&filter, publicKeyIndex, &result, &paginationCursor](const BlockIndexEntry& entry)
					{
						auto filterResult = entry.isMatchingFilter(filter, publicKeyIndex);
						if ((filterResult & FilterResult::USE) == FilterResult::USE) {
							if (paginationCursor >= filter.pagination.skipEntriesCount()) {
								result.push_back(entry.transactionNr);
							}
							paginationCursor++;
						}
						if (!filter.pagination.hasCapacityLeft(result.size()) || (filterResult & FilterResult::STOP) == FilterResult::STOP) {
							return false;
						}
						return true;
					}
				);
				return true;
			}
		);
		return result;
	}

	size_t BlockIndex::countTransactions(const gradido::blockchain::Filter& filter, const Dictionary& publicKeysDictionary) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		// prefilter, early exit
		if ((filter.minTransactionNr && filter.minTransactionNr > mMaxTransactionNr) ||
			(filter.maxTransactionNr && filter.maxTransactionNr < mMinTransactionNr)) {
			return 0;
		}

		uint32_t publicKeyIndex = 0;
		if (filter.involvedPublicKey && !filter.involvedPublicKey->isEmpty()) {
			auto involvedPublicKeyCopy = filter.involvedPublicKey->copyAsString();
			if (publicKeysDictionary.hasString(involvedPublicKeyCopy)) {
				publicKeyIndex = publicKeysDictionary.getIndexForString(involvedPublicKeyCopy);
			} else {
				// if public key not exist, no transaction can match
				return 0;
			}
		}

		auto interval = filteredTimepointInterval(filter);
		size_t result = 0;
		iterateRangeInOrder(interval.begin(), interval.end(), filter.searchDirection,
			[&](const date::year_month& intervalIt) -> bool
			{
				auto yearIt = mYearMonthAddressIndexEntries.find(intervalIt.year());
				if (yearIt == mYearMonthAddressIndexEntries.end()) {
					return true;
				}
				auto monthIt = yearIt->second.find(intervalIt.month());
				if (monthIt == yearIt->second.end()) {
					return true;
				}
				// iterate over entries in the month
				iterateRangeInOrder(monthIt->second.begin(), monthIt->second.end(), SearchDirection::ASC,
					[&](const BlockIndexEntry& entry) -> bool
					{
						auto filterResult = entry.isMatchingFilter(filter, publicKeyIndex);
						if ((filterResult & FilterResult::USE) == FilterResult::USE) {
							++result;
						}
						return true; // keep going
					}
				);
				return true;
			}
		);
		return result;
	}

	std::pair<uint64_t, uint64_t> BlockIndex::findTransactionsForMonthYear(date::year year, date::month month) const
	{
		std::lock_guard _lock(mRecursiveMutex);
		auto yearIt = mYearMonthAddressIndexEntries.find(year);
		if (yearIt == mYearMonthAddressIndexEntries.end()) {
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
		if (!mYearMonthAddressIndexEntries.size()) {
			return { date::year(0), date::month(0) };
		}
		auto firstEntry = mYearMonthAddressIndexEntries.begin();
		assert(firstEntry->second.size());
		return { firstEntry->first, firstEntry->second.begin()->first };
	}
	date::year_month BlockIndex::getNewestYearMonth() const
	{
		std::lock_guard _lock(mRecursiveMutex);
		if (!mYearMonthAddressIndexEntries.size()) {
			return { date::year(0), date::month(0) };
		}
		auto lastEntry = std::prev(mYearMonthAddressIndexEntries.end());
		assert(lastEntry->second.size());
		auto lastMonthEntry = std::prev(lastEntry->second.end());
		return { lastEntry->first, lastMonthEntry->first };
	}

	void BlockIndex::clearIndexEntries()
	{
		std::lock_guard _lock(mRecursiveMutex);
		for (auto& yearBlock : mYearMonthAddressIndexEntries) {
			for (auto& monthBlock : yearBlock.second) {
				for (auto& blockIndexEntry : monthBlock.second) {
					delete blockIndexEntry.addressIndices;
				}
			}
		}
		mYearMonthAddressIndexEntries.clear();
	}

	FilterResult BlockIndex::BlockIndexEntry::isMatchingFilter(const gradido::blockchain::Filter& filter, const uint32_t publicKeyIndex) const
	{
		if (filter.transactionType != TransactionType::NONE
			&& filter.transactionType != transactionType) {
			return FilterResult::DISMISS;
		}
		uint32_t coinCommunityKeyIndex = 0;
		if (!filter.coinCommunityId.empty()) {
			coinCommunityKeyIndex = FileBasedProvider::getInstance()->getCommunityIdIndex(filter.coinCommunityId);
		}
		if (coinCommunityKeyIndex && coinCommunityKeyIndex != coinCommunityIdIndex) {
			return FilterResult::DISMISS;
		}
		if (filter.minTransactionNr && filter.minTransactionNr > transactionNr) {
			return FilterResult::DISMISS;
		}
		if (filter.maxTransactionNr && filter.maxTransactionNr < transactionNr) {
			return FilterResult::DISMISS;
		}
		/*uint32_t publicKeyIndex = 0;
		if (filter.involvedPublicKey && !filter.involvedPublicKey->isEmpty()) {
			auto involvedPublicKeyCopy = filter.involvedPublicKey->copyAsString();
			if (publicKeysDictionary.hasString(involvedPublicKeyCopy)) {
				publicKeyIndex = publicKeysDictionary.getIndexForString(involvedPublicKeyCopy);
			}
		}*/
		if (publicKeyIndex) {
			bool found = false;
			for (int iPublicKeyIndices = 0; iPublicKeyIndices < addressIndiceCount; iPublicKeyIndices++) {
				if (publicKeyIndex == addressIndices[iPublicKeyIndices]) {
					found = true;
					break;
				}
			}
			if (!found) {
				return FilterResult::DISMISS;
			}
		}
		return FilterResult::USE;
	}

}