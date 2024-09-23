#include "TransactionList.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"

using namespace rapidjson;
using namespace gradido::interaction;

namespace model {
	namespace Apollo {

		TransactionList::TransactionList(
			std::shared_ptr<const gradido::blockchain::FileBased> blockchain,
			memory::ConstBlockPtr pubkey,
			rapidjson::Document::AllocatorType& alloc
		) : mBlockchain(blockchain), mPubkey(std::move(pubkey)), mJsonAllocator(alloc)
		{

		}

		Value TransactionList::generateList(
			std::vector<std::shared_ptr<const gradido::blockchain::TransactionEntry>> allTransactions,
			Timepoint now,
			int currentPage /*= 1*/,
			int pageSize /*= 25*/,
			bool orderDESC /*= true*/,
			bool onlyCreations /*= false*/			
		)
		{
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
					const std::shared_ptr<const gradido::blockchain::TransactionEntry>& x,
					const std::shared_ptr<const gradido::blockchain::TransactionEntry>& y) {
						return x->getTransactionNr() > y->getTransactionNr();
					});
			}
			else {
				// sort in ascending order
				std::sort(allTransactions.begin(), allTransactions.end(), [](
					const std::shared_ptr<const gradido::blockchain::TransactionEntry>& x,
					const std::shared_ptr<const gradido::blockchain::TransactionEntry>& y) {
						return x->getTransactionNr() < y->getTransactionNr();
					});
			}

			int entryIterator = 0;
			int pageIterator = 1;
			Timepoint balanceStartTimestamp(now);
			GradidoUnit balance;

			int countTransactions = 0;
			int foundRegisterAddressTransaction = 0;
			for (auto it = allTransactions.begin(); it != allTransactions.end(); it++) {
				if (entryIterator == pageSize) {
					pageIterator++;
					entryIterator = 0;
				}
				

				auto gradidoBlock = std::make_unique<gradido::data::ConfirmedTransaction>((*it)->getSerializedTransaction());
				auto transactionBody = gradidoBlock->getGradidoTransaction()->getTransactionBody();
				if (transactionBody->isRegisterAddress()) {
					auto registerAddress = transactionBody->getRegisterAddress();
					if (transactionBody->getOtherGroup().empty()) {
						if ((registerAddress->getAddressType() == gradido::data::AddressType::SUBACCOUNT && 
							registerAddress->getAccountPublicKey() == mPubkey) ||
							(registerAddress->getAddressType() != gradido::data::AddressType::SUBACCOUNT &&
							registerAddress->getUserPublicKey() == mPubkey)) {
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
						balanceStartTimestamp = gradidoBlock->getConfirmedAt();
					}

					if (pageIterator == currentPage) {
						transactionsVector.push_back(std::move(model::Apollo::Transaction(*gradidoBlock, mPubkey)));
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
						balanceStartTimestamp = gradidoBlock->getConfirmedAt();
					}
					entryIterator++;
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