#include "TransactionList.h"
#include "createTransaction/Context.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/NodeTransactionEntry.h"

#include "magic_enum/magic_enum.hpp"

using namespace rapidjson;
using namespace gradido::interaction;
using namespace gradido::blockchain;
using namespace magic_enum;

namespace model {
	namespace Apollo {

		TransactionList::TransactionList(
			std::shared_ptr<const gradido::blockchain::Abstract> blockchain,
			memory::ConstBlockPtr pubkey
		) : mBlockchain(blockchain), mPubkey(pubkey)
		{

		}

		Value TransactionList::generateList(Timepoint now, const Filter& filter, Document& root)
		{
			auto fileBasedBlockchain = std::dynamic_pointer_cast<const gradido::blockchain::FileBased>(mBlockchain);
			assert(fileBasedBlockchain);
			auto& alloc = root.GetAllocator();

			Value transactionList(kObjectType);
			transactionList.AddMember("balanceGDT", "0", alloc);
			// TODO: add number of active deferred transfers
			transactionList.AddMember("linkCount", 0, alloc);

			Value transactions(kArrayType);
			std::vector<model::Apollo::Transaction> transactionsVector;
			transactionsVector.reserve(filter.pagination.size);

			int countTransactions = fileBasedBlockchain->findAllResultCount(filter);
			transactionList.AddMember("count", countTransactions, alloc);
			auto addressType = mBlockchain->getAddressType(Filter(0,0,filter.involvedPublicKey));
			transactionList.AddMember("addressType", Value(enum_name(addressType).data(), alloc), alloc);			

			Filter filterCopy = filter;
			auto allTransactions = mBlockchain->findAll(filterCopy);
			if (!allTransactions.size()) {
				transactionList.AddMember("transactions", transactions, alloc);
				return std::move(transactionList);
			}
			
			// copy into vector to make reversing and loop through faster (cache-hit)
			std::vector<std::shared_ptr<const gradido::blockchain::TransactionEntry>> allTransactionsVector(allTransactions.begin(), allTransactions.end());
			allTransactions.clear();
			if (filter.searchDirection == SearchDirection::DESC) {
				std::reverse(allTransactionsVector.begin(), allTransactionsVector.end());
			}
			
			// all transaction is always sorted ASC, regardless of filter.searchDirection value
			GradidoUnit previousBalance(GradidoUnit::zero());
			Timepoint previousDate = mBlockchain->getStartDate();
			createTransaction::Context createTransactionContext(mBlockchain, addressType);
			for (auto& entry: allTransactionsVector)
			{
				auto confirmedTransaction = entry->getConfirmedTransaction();
				auto transactions = createTransactionContext.run(*confirmedTransaction, mPubkey);
				for (auto& transaction: transactions) {
					transactionsVector.push_back(transaction);
					transactionsVector.back().setPreviousBalance(
						previousBalance
					);
					if (previousBalance > GradidoUnit::zero()) {
						transactionsVector.back().calculateDecay(previousDate, confirmedTransaction->getConfirmedAt(), previousBalance);
					}
					previousDate = confirmedTransaction->getConfirmedAt();
					previousBalance = confirmedTransaction->getAccountBalance(mPubkey).getBalance();
				}				
			}
			allTransactionsVector.clear();

			// check if it has last decay
			auto& page = filter.pagination;
			if (countTransactions <= page.size || // less entries than possible on a page
				(filter.searchDirection == SearchDirection::ASC && page.skipEntriesCount() + page.size >= countTransactions) || // asc and on last page
				(filter.searchDirection == SearchDirection::DESC && !page.page) // desc and on first page
				) {
				auto& lastTransaction = transactionsVector.back();
				transactionsVector.push_back(model::Apollo::Transaction(
					lastTransaction.getDate(), 
					std::chrono::system_clock::now(), 
					lastTransaction.getBalance())
				);
			}
			// last decay 
			// last decay if ordered DESC
			if (transactionsVector.size() && filter.searchDirection == SearchDirection::DESC) {
				// reverse order of transactions in vector
				std::reverse(transactionsVector.begin(), transactionsVector.end());
			}
			
			// add transactions to array
			for (auto it = transactionsVector.begin(); it != transactionsVector.end(); it++) {
				transactions.PushBack(it->toJson(alloc), alloc);
			}

			transactionList.AddMember("transactions", transactions, alloc);
			return std::move(transactionList);
		}
	}
}