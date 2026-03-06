#include "BlockIndex.h"
#include "Exceptions.h"
#include "../controller/ControllerExceptions.h"
#include "../task/HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBasedProvider.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/blockchain/RangeUtils.h"
#include "gradido_blockchain/blockchain/SearchDirection.h"
#include "gradido_blockchain/data/TransactionType.h"
#include "gradido_blockchain/serialization/toJson.h"

#include "loguru/loguru.hpp"

using namespace rapidjson;
using gradido::blockchain::AbstractProvider, gradido::blockchain::Filter, gradido::blockchain::SearchDirection, gradido::blockchain::TransactionsIndex;
using gradido::data::TransactionType;

namespace cache {

	BlockIndex::BlockIndex(std::string_view groupFolderPath, uint32_t blockNr, uint32_t blockchainCommunityIdIndex)
		: TransactionsIndex(blockchainCommunityIdIndex), mFolderPath(groupFolderPath), mBlockNr(blockNr), mBlockchainCommunityIdIndex(blockchainCommunityIdIndex), mDirty(false)
	{
	}

	BlockIndex::~BlockIndex()
	{
		exit();
	}

	bool BlockIndex::init(const IDictionary<PublicKey>& publicKeysDictionary)
	{
		if (loadFromFile(publicKeysDictionary)) {
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
	}

	void BlockIndex::reset()
	{
		std::lock_guard _lock(mRecursiveMutex);
		clearIndexEntries();
		mTransactionNrsFileCursors.clear();		
		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr, mBlockchainCommunityIdIndex);
		// only needed if public key dictionary is again persistend
		// LOG_F(WARNING, "BlockIndex: %s was corrupted and must be rebuild", blockIndexFile.getFileName().c_str());
		blockIndexFile.reset();
		mMaxTransactionNr = 0;
		mMinTransactionNr = 0;
	}

	bool BlockIndex::loadFromFile(const IDictionary<PublicKey>& publicKeysDictionary)
	{
		std::lock_guard _lock(mRecursiveMutex);
		assert(!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size());

		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr, mBlockchainCommunityIdIndex);
		return blockIndexFile.readFromFile(this);
	}

	std::unique_ptr<model::files::BlockIndex> BlockIndex::serialize()
	{
		if (!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size() && !mMaxTransactionNr && !mMinTransactionNr) {
			// we haven't anything to save
			return nullptr;
		}
		
		assert(mYearMonthAddressIndexEntries.size() && mTransactionNrsFileCursors.size());
		auto blockIndexFile = std::make_unique<model::files::BlockIndex>(mFolderPath, mBlockNr, mBlockchainCommunityIdIndex);
		blockIndexFile->addYearBlock(mMinYearMonth.year());
		
		std::vector<uint32_t> publicKeyIndicesTemp;
		publicKeyIndicesTemp.reserve(10);
		for (auto monthYearIndex = 0; monthYearIndex < mYearMonthAddressIndexEntries.size(); monthYearIndex++)
		{
			auto monthYear = indexToYearMonth(monthYearIndex);
			if (monthYear.month() <= date::month(1) && mMinYearMonth != monthYear) {
				blockIndexFile->addYearBlock(monthYear.year());
			}
			blockIndexFile->addMonthBlock(monthYear.month());
			for (const auto& itEntry : mYearMonthAddressIndexEntries[monthYearIndex])
			{
				auto fileCursorIt = mTransactionNrsFileCursors.find(itEntry.transactionNr);
				if (fileCursorIt == mTransactionNrsFileCursors.end()) {
					throw GradidoNodeInvalidDataException("missing file cursor for transaction");
				}
				publicKeyIndicesTemp.clear();
				for (auto i = 0; i < itEntry.addressIndiceCount; i++) {
					publicKeyIndicesTemp.push_back(itEntry.addressIndices[i]);
				}
				blockIndexFile->addDataBlock(
					itEntry.transactionNr,
					fileCursorIt->second,
					itEntry.transactionType,
					itEntry.coinCommunityIdIndex,
					itEntry.isBalanceChanging,
					publicKeyIndicesTemp
				);
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
		uint16_t addressIndiceCount,
		uint8_t isBalanceChanging
	)
	{
		std::lock_guard _lock(mRecursiveMutex);
		mDirty = true;

		TransactionsIndex::addIndicesForTransaction(
			transactionType,
			coinCommunityIdIndex,
			year,
			month,
			transactionNr,
			addressIndices,
			addressIndiceCount,
			isBalanceChanging
		);

		addFileCursorForTransaction(transactionNr, fileCursor);		
		return true;
	}

	bool BlockIndex::addIndicesForTransaction(
		std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transactionEntry,
		IMutableDictionary<PublicKey>& publicKeyDictionary
	)
	{
		std::lock_guard _lock(mRecursiveMutex);
		TransactionsIndex::addIndicesForTransaction(transactionEntry, publicKeyDictionary);
		addFileCursorForTransaction(transactionEntry->getTransactionNr(), transactionEntry->getFileCursor());
		return true;
	}

	bool BlockIndex::addIndicesForTransaction(const gradido::data::compact::ConfirmedGradidoTx& compactTx)
	{
		std::lock_guard _lock(mRecursiveMutex);
		TransactionsIndex::addIndicesForTransaction(compactTx);
		return true;
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

	
}