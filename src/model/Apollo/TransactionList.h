#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H

#include "rapidjson/document.h"
#include "../../controller/Group.h"
#include "../../model/NodeTransactionEntry.h"

#include "Transaction.h"

namespace model {
	namespace Apollo {

		class TransactionList
		{
		public:
			TransactionList(Poco::SharedPtr<controller::Group> group, std::unique_ptr<std::string> pubkey, rapidjson::Document::AllocatorType&);

			rapidjson::Value generateList(
				std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> allTransactions,
				int currentPage = 1,
				int pageSize = 25,
				bool orderDESC = true,
				bool onlyCreations = false
			);
		protected:
			void calculateDecay(
				mpfr_ptr* balance, 
				Poco::Timestamp prevTransactionDate,
				model::Apollo::Transaction* currentTransaction
			);

			rapidjson::Value lastDecay(mpfr_ptr balance, Poco::Timestamp lastTransactionDate);

			Poco::SharedPtr<controller::Group> mGroup;
			std::unique_ptr<std::string> mPubkey;
			rapidjson::Document::AllocatorType& mJsonAllocator;

		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_LIST_H