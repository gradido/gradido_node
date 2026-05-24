#ifndef __GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H
#define __GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H

#include "../model/files/LevelDBWrapper.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"

namespace gradido {
	namespace data {
		class LedgerAnchor;
	}
}

namespace cache 
{
	// TODO: optimize
	class LedgerAnchor 
	{
	public:
		LedgerAnchor(std::string_view folder);
		~LedgerAnchor();

		// try to open db 
		//! \param cacheInBytes level db cache in bytes, 0 for no cache
		bool init(size_t cacheInBytes);
		void exit();
		//! remove state level db folder, clear maps
		void reset();

		void add(const gradido::data::LedgerAnchor& transactionId, uint64_t transactionNr);
		bool has(const gradido::data::LedgerAnchor& transactionId);
		//! \return 0 if not found, else transaction nr for message id
		uint64_t getTransactionNrForLedgerAnchor(const gradido::data::LedgerAnchor& transactionId);

	protected:
		//! read message id as key from level db and put into mMessageIdTransactionNrs if found
		//! \return 0 if not found or else transactionNr for message Id
		uint64_t readFromLevelDb(const std::string& ledgerAnchorSerialized);
		gradido::data::LedgerAnchor fromProtobuf(const std::string transactionIdString) const;
		std::string toProtobuf(const gradido::data::LedgerAnchor& transactionId) const;

		bool mInitalized;
		model::files::LevelDBWrapper mLevelDBFile;
		//! key is ledger anchor serialized with protopuf, value is transaction nr
		ExpireCache<std::string, uint64_t> mLedgerAnchorTransactionNrs;
	};
}

#endif //__GRADIDO_NODE_CACHE_HIERO_TRANSACTION_ID_H