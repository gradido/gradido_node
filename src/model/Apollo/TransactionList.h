#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H

#include "rapidjson/document.h"
#include "Transaction.h"

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
				memory::ConstBlockPtr pubkey,
				rapidjson::Document::AllocatorType& alloc
			);

			rapidjson::Value generateList(Timepoint now, const gradido::blockchain::Filter& filter);
		protected:
			void calculateDecay(
				GradidoUnit balance, 
				Timepoint prevTransactionDate,
				model::Apollo::Transaction* currentTransaction
			);

			rapidjson::Value lastDecay(GradidoUnit balance, Timepoint lastTransactionDate);

			std::shared_ptr<const gradido::blockchain::Abstract> mBlockchain;
			memory::ConstBlockPtr mPubkey;
			rapidjson::Document::AllocatorType& mJsonAllocator;

		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H