#include "TransactionList.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/NodeTransactionEntry.h"

using namespace rapidjson;
using namespace gradido::interaction;
using namespace gradido::blockchain;

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

			Timepoint balanceStartTimestamp(now);
			GradidoUnit balance((int64_t)0);

			int countTransactions = fileBasedBlockchain->findAllResultCount(filter);
			transactionList.AddMember("count", countTransactions, alloc);

			Filter filterCopy = filter;
			filterCopy.searchDirection = SearchDirection::ASC;
			auto allTransactions = mBlockchain->findAll(filterCopy);
			if (!allTransactions.size()) {
				transactionList.AddMember("transactions", transactions, alloc);
				return std::move(transactionList);
			}
			filterCopy.pagination = Pagination(1, 0);
			filterCopy.maxTransactionNr = allTransactions.front()->getTransactionNr() - 1;
			auto prevTransaction = mBlockchain->findOne(filterCopy);
			if (prevTransaction) {
				auto confirmedTransaction = prevTransaction->getConfirmedTransaction();
				balanceStartTimestamp = confirmedTransaction->getConfirmedAt();
				balance = confirmedTransaction->getAccountBalance();
			}
			
			// all transaction is always sorted ASC, regardless of filter.searchDirection value
			for (auto& entry: allTransactions)
			{
				auto confirmedTransaction = entry->getConfirmedTransaction();
				auto transactionBody = confirmedTransaction->getGradidoTransaction()->getTransactionBody();
				if (transactionBody->isRegisterAddress()) {
					continue;
				}

				transactionsVector.push_back(model::Apollo::Transaction(*confirmedTransaction, mPubkey));
				
				Timepoint prevTransactionDate(balanceStartTimestamp);
				if (transactionsVector.size() > 1) {
					prevTransactionDate = transactionsVector[transactionsVector.size() - 2].getDate();
					transactionsVector.back().setPreviousBalance(transactionsVector[transactionsVector.size() - 2].getBalance());
				}
				if (prevTransactionDate != now) {
					calculateDecay(balance, prevTransactionDate, &transactionsVector.back());
				}
			}
			// last decay if ordered DESC
			if (transactionsVector.size() && filter.searchDirection == SearchDirection::DESC) {
				transactions.PushBack(lastDecay(balance, transactionsVector.back().getDate(), root), alloc);
				// reverse order of transactions in vector
				std::reverse(transactionsVector.begin(), transactionsVector.end());
			}
			
			// add transactions to array
			for (auto it = transactionsVector.begin(); it != transactionsVector.end(); it++) {
				transactions.PushBack(it->toJson(alloc), alloc);
			}
			// last decay if ordered ASC
			if (transactionsVector.size() && filter.searchDirection == SearchDirection::ASC) {
				transactions.PushBack(lastDecay(balance, transactionsVector.back().getDate(), root), alloc);
			}

			transactionList.AddMember("transactions", transactions, alloc);
			return std::move(transactionList);
		}

		void TransactionList::calculateDecay(
			GradidoUnit balance,
			Timepoint prevTransactionDate,
			model::Apollo::Transaction* currentTransaction
		)
		{
			if (balance == GradidoUnit::zero()) {
				calculateAccountBalance::Context c(*mBlockchain);
				balance = c.run(mPubkey, prevTransactionDate);
			}
			currentTransaction->calculateDecay(prevTransactionDate, currentTransaction->getDate(), balance);
			balance += currentTransaction->getDecay()->getDecayAmount() + currentTransaction->getAmount();			
			currentTransaction->setBalance(balance);
		}

		Value TransactionList::lastDecay(GradidoUnit balance, Timepoint lastTransactionDate, rapidjson::Document& root)
		{
			model::Apollo::Transaction lastDecay(lastTransactionDate, std::chrono::system_clock::now(), balance);
			balance += lastDecay.getDecay()->getDecayAmount();
			
			return std::move(lastDecay.toJson(root.GetAllocator()));
		}
	}
}