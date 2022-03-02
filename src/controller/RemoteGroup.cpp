#include "RemoteGroup.h"

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: mGroupAlias(groupAlias), mGroupCoinColor(0)
	{
		// get coin color on first connect to remote group
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::getAllTransactions(std::function<bool(model::TransactionEntry*)> filter /*= nullptr*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::gradido::GradidoBlock> RemoteGroup::getLastTransaction()
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::getTransactionForId(uint64_t transactionId)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::findByMessageId(const MemoryBin* messageId, bool cachedOnly/* = true*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	uint64_t RemoteGroup::calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received)
	{
		throw std::runtime_error("not implemented yet");
	}

	uint32_t RemoteGroup::getGroupDefaultCoinColor() const
	{
		return mGroupCoinColor;
	}
}