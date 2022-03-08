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

		std::vector<Poco::SharedPtr<model::TransactionEntry>> getAllTransactions(std::function<bool(model::TransactionEntry*)> filter = nullptr);
		Poco::SharedPtr<model::gradido::GradidoBlock> getLastTransaction(std::function<bool(const model::gradido::GradidoBlock*)> filter = nullptr);
		mpfr_ptr calculateAddressBalance(const std::string& address, uint32_t coinColor, Poco::DateTime date);
		Poco::SharedPtr<model::TransactionEntry> getTransactionForId(uint64_t transactionId);
		Poco::SharedPtr<model::TransactionEntry> findLastTransactionForAddress(const std::string& address, uint32_t coinColor = 0);
		Poco::SharedPtr<model::TransactionEntry> findByMessageId(const MemoryBin* messageId, bool cachedOnly = true);
		void calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received, mpfr_ptr sum);
		uint32_t getGroupDefaultCoinColor() const;

	protected:
		std::string mGroupAlias;
		uint32_t    mGroupCoinColor;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H