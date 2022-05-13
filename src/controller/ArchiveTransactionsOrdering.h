#ifndef __GRADIDO_NODE_CONTROLLER_ARCHIVE_TRANSACTIONS_ORDERING_H
#define __GRADIDO_NODE_CONTROLLER_ARCHIVE_TRANSACTIONS_ORDERING_H

#include "../task/Thread.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"

#include <map>
#include <shared_mutex>

namespace controller
{
	class Group;

	/*
	 @author einhornimmond
	 @date   13.05.2022
	 @brief order archive transactions after transaction nr 

	 Used from puttransaction json-rpc call to make sure the order is correct, 
	 even if the caller of puttransaction uses multiple threads for calling with multiple transactions at once
	*/

	class ArchiveTransactionsOrdering : public task::Thread
	{
	public:
		ArchiveTransactionsOrdering(Group* parentGroup);
		~ArchiveTransactionsOrdering();

		void addPendingTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, uint64_t transactionNr);
	protected:
		int ThreadFunction();
		typedef std::map<uint64_t, std::unique_ptr<model::gradido::GradidoTransaction>> PendingTransactionsMap;
		PendingTransactionsMap mPendingTransactions;
		std::shared_mutex mPendingTransactionsMutex;
		Group* mParentGroup;
	};

	class ArchiveTransactionDoubletteException : public GradidoBlockchainException
	{
	public:
		explicit ArchiveTransactionDoubletteException(const char* what, uint64_t transactionNr) noexcept;
		std::string getFullString() const;
	protected:
		uint64_t mTransactionNr;
	};
}

#endif // __GRADIDO_NODE_CONTROLLER_ARCHIVE_TRANSACTIONS_ORDERING_H