#ifndef __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H

/*!
 * @author: Dario Rekowski
 * 
 * @date: 20.02.2022
 * 
 * @brief: Class for accessing blockchain with this server don't listen, it will be call the transaction from another Gradido Node Server which is listening to this group
 * 
 * 
 */

#include "gradido_blockchain/model/IGradidoBlockchain.h"

namespace controller {

	class RemoteGroup : public model::IGradidoBlockchain
	{
	public:
		RemoteGroup(const std::string& groupAlias);

		std::vector<std::shared_ptr<model::gradido::GradidoBlock>> getAllTransactions(model::gradido::TransactionType type);
		std::shared_ptr<model::gradido::GradidoBlock> getLastTransaction();
		std::shared_ptr<model::gradido::GradidoBlock> getTransactionForId(uint64_t transactionId);
		uint32_t getGroupDefaultCoinColor() const;

	protected:
		std::string mGroupAlias;
		uint32_t    mGroupCoinColor;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H