#include "TransactionList.h"
#include "createTransaction/Context.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "gradido_blockchain_core/types/transaction.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/NodeTransactionEntry.h"

#include "magic_enum/magic_enum.hpp"

using namespace rapidjson;
using namespace gradido::interaction;
using namespace gradido::blockchain;
using namespace magic_enum;
using gradido::data::Timestamp;
using serialization::toJsonString;

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
			uint32_t coinCommunityId = mBlockchain->getCommunityIdIndex();
			if (filter.coinCommunityIdIndex.has_value()) {
				coinCommunityId = filter.coinCommunityIdIndex.value();
			}
			assert(fileBasedBlockchain);
			auto& alloc = root.GetAllocator();

			Value transactionList(kObjectType);
			transactionList.AddMember("balanceGDT", "0", alloc);
			// TODO: add number of active deferred transfers
			transactionList.AddMember("linkCount", 0, alloc);

			Value transactions(kArrayType);
			std::vector<model::Apollo::Transaction> transactionsVector;
			transactionsVector.reserve(filter.pagination.size);

			auto filterOutNotForWallet = [&filter](const TransactionEntry& entry) -> FilterResult
			{
				// filter out creation transactions which this user has signed as moderator, and isn't the benefitor
				// shouldn't be needed any longer, because of Transaction Index change, using updatedBalancePublicKey instead of involvedPublicKey
				/*if (entry.isCreation()) {
					auto creation = entry.getTransactionBody()->getCreation();
					if (!creation->getRecipient().getPublicKey()->isTheSame(filter.involvedPublicKey)) {
						return FilterResult::DISMISS;
					}
				}*/
				// filter out register address transaction, because this won't show in wallet view
				if (entry.isRegisterAddress()) {
					return FilterResult::DISMISS;
				}
				return FilterResult::USE;
			};

			size_t countTransactions = 0;
			Filter countFilter = filter;
			countFilter.pagination = Pagination(0, 0);
			auto allTransactionsCount = mBlockchain->countAll(countFilter);
			countFilter.transactionType = GRDT_TRANSACTION_REGISTER_ADDRESS;
			auto registerAddressTransactionsCount = mBlockchain->countAll(countFilter);
			if (registerAddressTransactionsCount < allTransactionsCount) {
				countTransactions = allTransactionsCount - registerAddressTransactionsCount;
			}

			auto addressType = mBlockchain->getAddressType(Filter(0,0,filter.updatedBalancePublicKey));
			transactionList.AddMember("addressType", Value(enum_name(addressType).data(), alloc), alloc);

			Filter filterCopy = filter;
			filterCopy.filterFunction = filterOutNotForWallet;
			auto allTransactions = mBlockchain->findAll(filterCopy);

			transactionList.AddMember("count", countTransactions, alloc);
			if (!allTransactions.size()) {
				transactionList.AddMember("transactions", transactions, alloc);
				return std::move(transactionList);
			}

			if (filter.searchDirection == SearchDirection::DESC) {
				std::reverse(allTransactions.begin(), allTransactions.end());
			}

			// all transaction is always sorted ASC, regardless of filter.searchDirection value
			GradidoUnit previousBalance(GradidoUnit::zero());
			// load previous balance before first transaction for decay
			Timepoint previousDate = mBlockchain->getStartDate();
			auto firstTransactionNr = allTransactions.front()->getTransactionNr();
			if (firstTransactionNr > 1) {
				const auto& previousTransactionDate = allTransactions.front()->getConfirmedTransaction()->getConfirmedAt();
				auto beforePreviousTransactionDate = Timestamp(
					previousTransactionDate.getSeconds(),
					previousTransactionDate.getNanos() - 1000
				);

				Filter previousTransactionFilter = Filter::LAST_TRANSACTION;
				previousTransactionFilter.maxTransactionNr = firstTransactionNr - 1;
				previousTransactionFilter.updatedBalancePublicKey = filter.updatedBalancePublicKey;
				previousTransactionFilter.timepointInterval = TimepointInterval(previousDate, beforePreviousTransactionDate);
				auto previousTransaction = mBlockchain->findOne(previousTransactionFilter);
				
				if (previousTransaction) {
					auto accountBalance = previousTransaction->getConfirmedTransaction()->getAccountBalance(mPubkey, coinCommunityId);
					if (accountBalance.getBalance() > GradidoUnit::zero()) {
						previousBalance = accountBalance.getBalance();
						previousDate = previousTransaction->getConfirmedTransaction()->getConfirmedAt();
					}
				}
			}

			createTransaction::Context createTransactionContext(mBlockchain, addressType);
			for (auto& entry: allTransactions)
			{
				auto confirmedTransaction = entry->getConfirmedTransaction();
				auto transactions = createTransactionContext.run(*confirmedTransaction, mPubkey);
				for (auto& transaction: transactions) {
					transactionsVector.push_back(transaction);
					// TODO: choose correct coin color
					auto balance = confirmedTransaction->getAccountBalance(mPubkey, coinCommunityId);
					transactionsVector.back().setPreviousBalance(
						previousBalance
					);
					if (previousBalance > GradidoUnit::zero()) {
						transactionsVector.back().setDecay(previousDate, confirmedTransaction->getConfirmedAt(), previousBalance);
					}
					previousDate = confirmedTransaction->getConfirmedAt();
					previousBalance = transactionsVector.back().getBalance();
				}
			}
			allTransactions.clear();
			if (transactionsVector.empty()) {
				transactionList.AddMember("transactions", Value(kArrayType), alloc);
				return std::move(transactionList);
			}

			// check if it has last decay
			auto& page = filter.pagination;
			if (countTransactions <= page.size || // less entries than possible on a page
				(filter.searchDirection == SearchDirection::ASC && page.skipEntriesCount() + page.size >= countTransactions) || // asc and on last page
				(filter.searchDirection == SearchDirection::DESC && page.page <= 1) // desc and on first page
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