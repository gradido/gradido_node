#include "RemoteGroup.h"

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: mGroupAlias(groupAlias), mGroupCoinColor(0)
	{
		// get coin color on first connect to remote group
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::getAllTransactions(std::function<bool(model::TransactionEntry*)> filter /*= nullptr*/)
	{
		return {};
	}

	Poco::SharedPtr<model::gradido::GradidoBlock> RemoteGroup::getLastTransaction()
	{
		return nullptr;
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::getTransactionForId(uint64_t transactionId)
	{
		return nullptr;
	}

	Poco::SharedPtr<model::TransactionEntry> findByMessageId(const MemoryBin* messageId, bool cachedOnly/* = true*/)
	{
		return nullptr;
	}

	uint32_t RemoteGroup::getGroupDefaultCoinColor() const
	{
		return mGroupCoinColor;
	}
}