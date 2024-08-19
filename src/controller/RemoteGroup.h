#ifndef __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H

#include "gradido_blockchain/model/IGradidoBlockchain.h"

namespace controller {

/*!
  @author Dario Rekowski
 
  @date 20.02.2022
 
  @brief Class for accessing blockchain with this server don't listen, it will be call the transaction from another Gradido Node Server which is listening to this group 
 
 */

	class RemoteGroup : public model::IGradidoBlockchain
	{
	public:
		RemoteGroup(const std::string& groupAlias);

		std::vector<std::shared_ptr<gradido::blockchain::TransactionEntry>> searchTransactions(
			uint64_t startTransactionNr = 0,
			std::function<FilterResult(model::TransactionEntry*)> filter = nullptr,
			SearchDirection order = SearchDirection::ASC
		);
		Poco::SharedPtr<model::gradido::ConfirmedTransaction> getLastTransaction(std::function<bool(const model::gradido::ConfirmedTransaction*)> filter = nullptr);
		mpfr_ptr calculateAddressBalance(const std::string& address, const std::string& coinGroupId, Poco::DateTime date, uint64_t ownTransactionNr);
		proto::gradido::RegisterAddress_AddressType getAddressType(const std::string& address);
		std::shared_ptr<gradido::blockchain::TransactionEntry> getTransactionForId(uint64_t transactionId);
		std::shared_ptr<gradido::blockchain::TransactionEntry> findLastTransactionForAddress(const std::string& address, const std::string& coinGroupId = "");
		std::shared_ptr<gradido::blockchain::TransactionEntry> findByMessageId(const MemoryBin* messageId, bool cachedOnly = true);
		std::vector<std::shared_ptr<gradido::blockchain::TransactionEntry>> findTransactions(const std::string& address);
		//! \brief Find transactions of account from a specific month.
		//! \param address User account public key.
		std::vector<std::shared_ptr<gradido::blockchain::TransactionEntry>> findTransactions(const std::string& address, int month, int year);
		std::vector<std::shared_ptr<gradido::blockchain::TransactionEntry>> findTransactions(const std::string& address, uint32_t maxResultCount, uint64_t startTransactionNr);
		const std::string& getCommunityId() const;

	protected:
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H