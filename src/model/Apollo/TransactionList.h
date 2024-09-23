#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H

#include "rapidjson/document.h"
#include "../../blockchain/FileBased.h"
#include "../../model/NodeTransactionEntry.h"

#include "Transaction.h"

namespace model {
	namespace Apollo {

		class TransactionList
		{
		public:
			TransactionList(
				std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
				memory::ConstBlockPtr pubkey,
				rapidjson::Document::AllocatorType& alloc
			);

			rapidjson::Value generateList(
				std::vector<std::shared_ptr<const gradido::blockchain::TransactionEntry>> allTransactions,
				Timepoint now,
				int currentPage = 1,
				int pageSize = 25,
				bool orderDESC = true,
				bool onlyCreations = false
			);
		protected:
			void calculateDecay(
				GradidoUnit balance, 
				Timepoint prevTransactionDate,
				model::Apollo::Transaction* currentTransaction
			);

			rapidjson::Value lastDecay(GradidoUnit balance, Timepoint lastTransactionDate);

			std::shared_ptr<const gradido::blockchain::FileBased> mBlockchain;
			memory::ConstBlockPtr mPubkey;
			rapidjson::Document::AllocatorType& mJsonAllocator;

		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H