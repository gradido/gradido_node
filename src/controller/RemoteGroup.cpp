#include "RemoteGroup.h"

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: mGroupAlias(groupAlias)
	{
		// get coin color on first connect to remote group
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::searchTransactions(
		uint64_t startTransactionNr/* = 0*/,
		std::function<FilterResult(model::TransactionEntry*)> filter/* = nullptr*/,
		SearchDirection order /*= SearchDirection::ASC*/
	)
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
	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::findTransactions(const std::string& address)
	{
		throw std::runtime_error("not implemented yet");
	}

	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::findTransactions(const std::string& address, int month, int year)
	{
		throw std::runtime_error("not implemented yet");
	}
	std::vector<Poco::SharedPtr<model::TransactionEntry>> RemoteGroup::findTransactions(const std::string& address, uint32_t maxResultCount, uint64_t startTransactionNr)
	{
		throw std::runtime_error("not implemented yet");
	}

	const std::string& RemoteGroup::getGroupId() const
	{
		return mGroupAlias;
	}
}