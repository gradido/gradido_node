#ifndef __GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H
#define __GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H

#include "../model/files/LevelDBWrapper.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"

namespace cache
{
	class HieroTransactionId 
	{
	public:
		HieroTransactionId(std::string_view folder);
		~HieroTransactionId();

		// try to open db 
		//! \param cacheInBytes level db cache in bytes, 0 for no cache
		bool init(size_t cacheInBytes);
		void exit();
		//! remove state level db folder, clear maps
		void reset();

		void add(memory::ConstBlockPtr transactionId, uint64_t transactionNr);
		bool has(memory::ConstBlockPtr transactionId);
		//! \return 0 if not found, else transaction nr for message id
		uint64_t getTransactionNrForHieroTransactionId(memory::ConstBlockPtr messageId);

	protected:
		//! read message id as key from level db and put into mMessageIdTransactionNrs if found
		//! \return 0 if not found or else transactionNr for message Id
		uint64_t readFromLevelDb(memory::ConstBlockPtr transactionId, hiero::TransactionId transactionIdObj);
		hiero::TransactionId fromProtobuf(memory::ConstBlockPtr transactionId) const;

		bool mInitalized;
		model::files::LevelDBWrapper mLevelDBFile;
		//! key is iota message id, value is transaction nr
		ExpireCache<hiero::TransactionId, uint64_t> mHieroTransactionIdTransactionNrs;
	};
}

#endif //__GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H