#include "RemoteGroup.h"

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: mGroupAlias(groupAlias), mGroupCoinColor(0)
	{
		// get coin color on first connect to remote group
	}

	std::vector<std::shared_ptr<model::gradido::GradidoBlock>> RemoteGroup::getAllTransactions(model::gradido::TransactionType type)
	{
		return {};
	}

	std::shared_ptr<model::gradido::GradidoBlock> RemoteGroup::getLastTransaction()
	{
		return nullptr;
	}

	std::shared_ptr<model::gradido::GradidoBlock> RemoteGroup::getTransactionForId(uint64_t transactionId)
	{
		return nullptr;
	}

	uint32_t RemoteGroup::getGroupDefaultCoinColor() const
	{
		return mGroupCoinColor;
	}
}