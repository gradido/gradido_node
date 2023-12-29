#include "TransactionList.h"

#include "gradido_blockchain/MemoryManager.h"

using namespace rapidjson;

namespace model {
	namespace Apollo {

		TransactionList::TransactionList(Poco::SharedPtr<controller::Group> group, std::unique_ptr<std::string> pubkey, rapidjson::Document::AllocatorType& alloc)
			: mGroup(group), mPubkey(std::move(pubkey)), mJsonAllocator(alloc)
		{

		}

		Value TransactionList::generateList(
			std::vector<Poco::SharedPtr<model::TransactionEntry>> allTransactions,
			Poco::Timestamp now,
			int currentPage /*= 1*/,
			int pageSize /*= 25*/,
			bool orderDESC /*= true*/,
			bool onlyCreations /*= false*/			
		)
		{
			auto mm = MemoryManager::getInstance();
			Value transactionList(kObjectType);
			transactionList.AddMember("balanceGDT", "0", mJsonAllocator);			
			// TODO: add number of active deferred transfers
			transactionList.AddMember("linkCount", 0, mJsonAllocator);

			Value transactions(kArrayType);
			std::vector<model::Apollo::Transaction> transactionsVector;
			transactionsVector.reserve(pageSize);

			if (orderDESC) {
				// sort in descending order
				std::sort(allTransactions.begin(), allTransactions.end(), [](
					const Poco::SharedPtr<model::TransactionEntry>& x,
					const Poco::SharedPtr<model::TransactionEntry>& y) {
						return x->getTransactionNr() > y->getTransactionNr();
					});
			}
			else {
				// sort in ascending order
				std::sort(allTransactions.begin(), allTransactions.end(), [](
					const Poco::SharedPtr<model::TransactionEntry>& x,
					const Poco::SharedPtr<model::TransactionEntry>& y) {
						return x->getTransactionNr() < y->getTransactionNr();
					});
			}

			int entryIterator = 0;
			int pageIterator = 1;
			Poco::Timestamp balanceStartTimestamp(now);
			mpfr_ptr balance = nullptr;

			int countTransactions = 0;
			int foundRegisterAddressTransaction = 0;
			for (auto it = allTransactions.begin(); it != allTransactions.end(); it++) {
				if (entryIterator == pageSize) {
					pageIterator++;
					entryIterator = 0;
				}
				

				auto gradidoBlock = std::make_unique<model::gradido::ConfirmedTransaction>((*it)->getSerializedTransaction());
				auto transactionBody = gradidoBlock->getGradidoTransaction()->getTransactionBody();
				if (transactionBody->isRegisterAddress()) {
					auto registerAddress = transactionBody->getRegisterAddress();
					if (transactionBody->isLocal()) {
						if ((registerAddress->isSubaccount() && registerAddress->getAccountPubkeyString() == *mPubkey.get()) ||
							(!registerAddress->isSubaccount() && registerAddress->getUserPubkeyString() == *mPubkey.get())) {
							foundRegisterAddressTransaction = 1;
						}
					}
					else {
						// for moving transactions
						assert(false || "is not implemented yet");
					}
				}
				
				if (!onlyCreations && (transactionBody->isTransfer() || transactionBody->isDeferredTransfer())
					|| transactionBody->isCreation()) {
					countTransactions++;

					if (!orderDESC && pageIterator == currentPage - 1) {
						// if order asc get last timestamp from previous page
						balanceStartTimestamp = gradidoBlock->getReceivedAsTimestamp();
					}

					if (pageIterator == currentPage) {
						transactionsVector.push_back(std::move(model::Apollo::Transaction(gradidoBlock.get(), *mPubkey.get())));
						if ((it + foundRegisterAddressTransaction == allTransactions.begin() && !orderDESC) ||
							(orderDESC && (it + 1 + foundRegisterAddressTransaction) == allTransactions.end()))
						{
							transactionsVector.back().setFirstTransaction(true);
							balance = mm->getMathMemory();
							mpfr_set(balance, transactionsVector.back().getAmount(), gDefaultRound);
						}
						if (!orderDESC) {
							Poco::Timestamp prevTransactionDate(now);
							if (transactionsVector.size() > 1) {
								prevTransactionDate = transactionsVector[transactionsVector.size() - 2].getDate();
							}
							else if (!transactionsVector.front().isFirstTransaction()) {
								prevTransactionDate = balanceStartTimestamp;
							}
							if (prevTransactionDate != now) {
								calculateDecay(&balance, prevTransactionDate, &transactionsVector.back());
							}
						}

					}
					if (orderDESC && pageIterator == currentPage + 1 && !entryIterator) {
						balanceStartTimestamp = gradidoBlock->getReceivedAsTimestamp();
					}
					entryIterator++;
				}
			}
			if (foundRegisterAddressTransaction && orderDESC && !balance && transactionsVector.size()) {
				transactionsVector.back().setFirstTransaction(true);
				balance = mm->getMathMemory();
				mpfr_set(balance, transactionsVector.back().getAmount(), gDefaultRound);
			}

			transactionList.AddMember("count", countTransactions, mJsonAllocator);
			if (orderDESC && transactionsVector.size()) {
				for (auto it = transactionsVector.rbegin(); it != transactionsVector.rend(); it++) {
					Poco::Timestamp prevTransactionDate(now);

					if (it != transactionsVector.rbegin()) {
						prevTransactionDate = (it - 1)->getDate();
					}
					else if (!transactionsVector.back().isFirstTransaction()) {
						prevTransactionDate = balanceStartTimestamp;
					}
					if (prevTransactionDate != now) {
						calculateDecay(&balance, prevTransactionDate, &*it);
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
			mm->releaseMathMemory(balance);

			transactionList.AddMember("transactions", transactions, mJsonAllocator);
			return std::move(transactionList);
		}

		void TransactionList::calculateDecay(
			mpfr_ptr* balance,
			Poco::Timestamp prevTransactionDate,
			model::Apollo::Transaction* currentTransaction
		)
		{
			if (!*balance) {
				*balance = mGroup->calculateAddressBalance(*mPubkey.get(), 0, prevTransactionDate, mGroup->getLastTransaction()->getID() + 1);
			}
			currentTransaction->calculateDecay(prevTransactionDate, currentTransaction->getDate(), *balance);
			mpfr_add(*balance, *balance, currentTransaction->getDecay()->getDecayAmount(), gDefaultRound);
			mpfr_add(*balance, *balance, currentTransaction->getAmount(), gDefaultRound);			
			currentTransaction->setBalance(*balance);
		}

		Value TransactionList::lastDecay(mpfr_ptr balance, Poco::Timestamp lastTransactionDate)
		{
			model::Apollo::Transaction lastDecay(lastTransactionDate, Poco::Timestamp(), balance);
			mpfr_add(balance, balance, lastDecay.getDecay()->getDecayAmount(), gDefaultRound);
			
			return std::move(lastDecay.toJson(mJsonAllocator));
		}
	}
}