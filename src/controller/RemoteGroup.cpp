#include "RemoteGroup.h"

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: mGroupAlias(groupAlias)
	{
		// get coin color on first connect to remote group
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::getAllTransactions(std::function<bool(model::TransactionEntry*)> filter /*= nullptr*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::gradido::GradidoBlock> RemoteGroup::getLastTransaction(std::function<bool(const model::gradido::GradidoBlock*)> filter /*= nullptr*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	mpfr_ptr RemoteGroup::calculateAddressBalance(const std::string& address, const std::string& coinGroupId, Poco::DateTime date)
	{
		throw std::runtime_error("not implemented yet");
	}

	proto::gradido::RegisterAddress_AddressType RemoteGroup::getAddressType(const std::string& address)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::getTransactionForId(uint64_t transactionId)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::findLastTransactionForAddress(const std::string& address, const std::string& coinGroupId/* = ""*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	Poco::SharedPtr<model::TransactionEntry> RemoteGroup::findByMessageId(const MemoryBin* messageId, bool cachedOnly/* = true*/)
	{
		throw std::runtime_error("not implemented yet");
	}

	void RemoteGroup::calculateCreationSum(const std::string& address, int month, int year, Poco::DateTime received, mpfr_ptr sum)
	{
		throw std::runtime_error("not implemented yet");
	}

	const std::string& RemoteGroup::getGroupId() const
	{
		return mGroupAlias;
	}
}