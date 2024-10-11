#include "TransactionList.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"

#include "../../blockchain/FileBased.h"

using namespace rapidjson;
using namespace gradido::interaction;

namespace model {
	namespace Apollo {

		TransactionList::TransactionList(
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
			memory::ConstBlockPtr pubkey,
			rapidjson::Document::AllocatorType& alloc
		) : mBlockchain(blockchain), mPubkey(pubkey), mJsonAllocator(alloc)
		{

		}

		Value TransactionList::generateList(
			std::vector<std::shared_ptr<const gradido::blockchain::TransactionEntry>> allTransactions,
			Timepoint now,
			const gradido::blockchain::Filter& filter
		)
		{
			auto fileBasedBlockchain = std::dynamic_pointer_cast<gradido::blockchain::FileBased>(mBlockchain);
			assert(fileBasedBlockchain);

			Value transactionList(kObjectType);
			transactionList.AddMember("balanceGDT", "0", mJsonAllocator);			
			// TODO: add number of active deferred transfers
			transactionList.AddMember("linkCount", 0, mJsonAllocator);

			Value transactions(kArrayType);
			std::vector<model::Apollo::Transaction> transactionsVector;
			transactionsVector.reserve(filter.pagination.size);

			Timepoint balanceStartTimestamp(now);
			GradidoUnit balance;

			int countTransactions = fileBasedBlockchain->findAllResultCount(filter);
			int foundRegisterAddressTransaction = 0;
			for (auto it = allTransactions.begin(); it != allTransactions.end(); it++) 
			{
				auto confirmedTransaction = std::make_unique<gradido::data::ConfirmedTransaction>((*it)->getSerializedTransaction());
				auto transactionBody = confirmedTransaction->getGradidoTransaction()->getTransactionBody();
				if (transactionBody->isRegisterAddress()) {
					continue;
				}
				
				if (!orderDESC && pageIterator == currentPage - 1) {
					// if order asc get last timestamp from previous page
					balanceStartTimestamp = confirmedTransaction->getConfirmedAt();
				}

				if (pageIterator == currentPage) {
					transactionsVector.push_back(std::move(model::Apollo::Transaction(*confirmedTransaction, mPubkey)));
					if ((it + foundRegisterAddressTransaction == allTransactions.begin() && !orderDESC) ||
						(orderDESC && (it + 1 + foundRegisterAddressTransaction) == allTransactions.end()))
					{
						transactionsVector.back().setFirstTransaction(true);
						balance = transactionsVector.back().getAmount();
					}
					if (!orderDESC) {
						Timepoint prevTransactionDate(now);
						if (transactionsVector.size() > 1) {
							prevTransactionDate = transactionsVector[transactionsVector.size() - 2].getDate();
						}
						else if (!transactionsVector.front().isFirstTransaction()) {
							prevTransactionDate = balanceStartTimestamp;
						}
						if (prevTransactionDate != now) {
							calculateDecay(balance, prevTransactionDate, &transactionsVector.back());
						}
					}

				}
				if (orderDESC && pageIterator == currentPage + 1 && !entryIterator) {
					balanceStartTimestamp = confirmedTransaction->getConfirmedAt();
				}
			}
			if (foundRegisterAddressTransaction && orderDESC && !balance && transactionsVector.size()) {
				transactionsVector.back().setFirstTransaction(true);
				balance = transactionsVector.back().getAmount();
			}

			transactionList.AddMember("count", countTransactions, mJsonAllocator);
			if (orderDESC && transactionsVector.size()) {
				for (auto it = transactionsVector.rbegin(); it != transactionsVector.rend(); it++) {
					Timepoint prevTransactionDate(now);

					if (it != transactionsVector.rbegin()) {
						prevTransactionDate = (it - 1)->getDate();
					}
					else if (!transactionsVector.back().isFirstTransaction()) {
						prevTransactionDate = balanceStartTimestamp;
					}
					if (prevTransactionDate != now) {
						calculateDecay(balance, prevTransactionDate, &*it);
					}
				}
			}

			// last decay if ordered DESC
			if (orderDESC && transactionsVector.size()) {
				transactions.PushBack(lastDecay(balance, transactionsVector.front().getDate()), mJsonAllocator);
			}
			// add transactions to array
			for (auto it = transactionsVector.begin(); it != transactionsVector.end(); it++) {
				transactions.PushBack(it->toJson(mJsonAllocator), mJsonAllocator);
			}
			// last decay if ordered ASC
			if (!orderDESC && transactionsVector.size()) {
				transactions.PushBack(lastDecay(balance, transactionsVector.back().getDate()), mJsonAllocator);
			}

			transactionList.AddMember("transactions", transactions, mJsonAllocator);
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
				balance = c.run(
					mPubkey,
					prevTransactionDate,
					mBlockchain->findOne(gradido::blockchain::Filter::LAST_TRANSACTION)->getTransactionNr() + 1
				);
			}
			currentTransaction->calculateDecay(prevTransactionDate, currentTransaction->getDate(), balance);
			balance += currentTransaction->getDecay()->getDecayAmount() + currentTransaction->getAmount();			
			currentTransaction->setBalance(balance);
		}

		Value TransactionList::lastDecay(GradidoUnit balance, Timepoint lastTransactionDate)
		{
			model::Apollo::Transaction lastDecay(lastTransactionDate, Timepoint(), balance);
			balance += lastDecay.getDecay()->getDecayAmount();
			
			return std::move(lastDecay.toJson(mJsonAllocator));
		}
	}
}