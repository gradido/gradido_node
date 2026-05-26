#ifndef GRADIDO_NODE_SERVER_JSON_RPC_WIRE_FILTER_H
#define GRADIDO_NODE_SERVER_JSON_RPC_WIRE_FILTER_H

#include "gradido_blockchain/types.h"
#include "gradido_blockchain/blockchain/CompactFilter.h"
#include "gradido_blockchain/blockchain/Pagination.h"
#include "gradido_blockchain/blockchain/PublicKeySearchType.h"
#include "gradido_blockchain/blockchain/SearchDirection.h"
#include "gradido_blockchain/data/ByteArray.h"
#include "gradido_blockchain/lib/TimepointInterval.h"
#include "gradido_blockchain_core/types/transaction.h"

#include <optional>
#include <string>

namespace gradido {
	class AppContext;
}

namespace server::json_rpc {

	enum class WireOutputFormat {
		Base64, // protobuf serialized, encoded in base64
		Json // json format, encoded in string
	};

  struct WireFilter 
  {
		WireFilter();
		//! search direction and result order, default: DESC
		gradido::blockchain::SearchDirection searchDirection;

		//! transaction type
		grdt_transaction transactionType;

		//! type of data publicKey contains
		gradido::blockchain::PublicKeySearchType publicKeySearchType;

		//! format for result returned to caller
		WireOutputFormat format;

		//! index starts with 1
		std::string communityId;
		//! for colored coins, index starts with 1
		std::string coinCommunityId;

		//! transaction number to stop search, 0 means no stop 
		uint64_t maxTransactionNr;
		//! transaction number to start from, 0 default
		uint64_t minTransactionNr;

		//! return only transaction in which the public key is involved, either directly in the transaction or as signer
		gradido::data::PublicKey publicKey;

		//! search result scope 
		gradido::blockchain::Pagination pagination;
		//! interval between two dates with 1 month resolution
		TimepointInterval timepointInterval;

		//! for the future, but not yet implemented
		std::string luaFilterFunction;

		gradido::blockchain::CompactFilter toCompactFilter(gradido::AppContext& appContext) const;
  };
}

#endif // GRADIDO_NODE_SERVER_JSON_RPC_WIRE_FILTER_H