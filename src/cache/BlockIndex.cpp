#include "BlockIndex.h"
#include "Exceptions.h"
#include "../controller/ControllerExceptions.h"
#include "../task/HddWriteBufferTask.h"
#include "../ServerGlobals.h"
#include "../blockchain/FileBasedProvider.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/blockchain/RangeUtils.h"
#include "gradido_blockchain/blockchain/SearchDirection.h"
#include "gradido_blockchain/data/ByteArray.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"
#include "gradido_blockchain/serialization/toJson.h"

#include "loguru/loguru.hpp"

using namespace rapidjson;
using gradido::blockchain::AbstractProvider;
using gradido::blockchain::Filter, gradido::blockchain::SearchDirection;
using gradido::blockchain::TransactionsIndexRoaringBitmaps;
using gradido::data::compact::ConfirmedGradidoTx;
using gradido::data::PublicKey;

namespace cache {

	BlockIndex::BlockIndex(std::string_view groupFolderPath, uint32_t blockNr, uint32_t blockchainCommunityIdIndex)
		: TransactionsIndexRoaringBitmaps(blockchainCommunityIdIndex), mFolderPath(groupFolderPath), mBlockNr(blockNr), mBlockchainCommunityIdIndex(blockchainCommunityIdIndex), mDirty(false)
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
		reset();
		mTransactionNrsFileCursors.clear();
	}

	void BlockIndex::reset()
	{
		std::lock_guard _lock(mRecursiveMutex);
		TransactionsIndexRoaringBitmaps::reset();
		mTransactionNrsFileCursors.clear();		
		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr, mBlockchainCommunityIdIndex);
		// only needed if public key dictionary is again persistend
		// LOG_F(WARNING, "BlockIndex: %s was corrupted and must be rebuild", blockIndexFile.getFileName().c_str());
		blockIndexFile.reset();
	}

	bool BlockIndex::loadFromFile(const IDictionary<PublicKey>& publicKeysDictionary)
	{
		return false;
		/*
		* // need to be rewritten
		std::lock_guard _lock(mRecursiveMutex);
		assert(!mYearMonthAddressIndexEntries.size() && !mTransactionNrsFileCursors.size());

		model::files::BlockIndex blockIndexFile(mFolderPath, mBlockNr, mBlockchainCommunityIdIndex);
		return blockIndexFile.readFromFile(this);
		*/
	}

	bool BlockIndex::writeIntoFile()
	{
		return false;
	}


	bool BlockIndex::addIndicesForTransaction(const ConfirmedGradidoTx& compactTx, const IDictionary<PublicKey>& publicKeyDict)
	{
		std::lock_guard _lock(mRecursiveMutex);
		TransactionsIndexRoaringBitmaps::addTransactionIndices(compactTx, publicKeyDict);
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