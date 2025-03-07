#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H

#include "Transaction.h"
#include "rapidjson/document.h"

namespace gradido {
	namespace blockchain {
		class Abstract;
		class TransactionEntry;
		class Filter;
	}
}

namespace model {
	namespace Apollo {

		class TransactionList
		{
		public:
			TransactionList(
				std::shared_ptr<const gradido::blockchain::Abstract> blockchain,
				memory::ConstBlockPtr pubkey
			);
			rapidjson::Value generateList(Timepoint now, const gradido::blockchain::Filter& filter, rapidjson::Document& root);
		protected:

			std::shared_ptr<const gradido::blockchain::Abstract> mBlockchain;
			memory::ConstBlockPtr mPubkey;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H