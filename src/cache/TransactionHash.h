#ifndef __GRADIDO_NODE_CACHE_TRANSACTION_HASH_H
#define __GRADIDO_NODE_CACHE_TRANSACTION_HASH_H

#include "gradido_blockchain/crypto/SignatureOctet.h"
#include "gradido_blockchain/lib/ExpireCache.h"
#include "gradido_blockchain/const.h"

#include <unordered_map>

namespace gradido {
	namespace data {
		class GradidoTransaction;
		class ConfirmedTransaction;
	}
}

#define MAGIC_NUMBER_SIGNATURE_CACHE std::max(std::chrono::seconds(600), MAGIC_NUMBER_MAX_TIMESPAN_BETWEEN_CREATING_AND_RECEIVING_TRANSACTION)

namespace cache {
	/*!
	* @author einhornimmond
	* @date 2024-10-18
	* Cache for storing transaction hashes for fast doublettes check
	*/
	class TransactionHash
	{
	public:
		TransactionHash(std::string_view communityId);
		virtual ~TransactionHash();

		void push(const gradido::data::ConfirmedTransaction& confirmedTransaction);
		bool has(const gradido::data::GradidoTransaction& gradidoTransaction) const;
	protected:
		//! key is first 8 Byte from Transaction Signature, the distribution on ed25519 signatures should be good enough even by using only the first 8 Bytes
		//! data are transaction nr
		ExpireCache<SignatureOctet, uint64_t> mSignaturePartTransactionNrs;
		std::string mCommunityId;
	};
}

#endif //__GRADIDO_NODE_CACHE_TRANSACTION_HASH_H