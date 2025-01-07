#include "RemoteGroup.h"

using namespace gradido::blockchain;
using namespace gradido::data;

namespace controller {
	RemoteGroup::RemoteGroup(const std::string& groupAlias)
		: Abstract(groupAlias)
	{
		// get coin color on first connect to remote group
	}
	//! validate and generate confirmed transaction
	//! throw if gradido transaction isn't valid
	//! \return false if transaction already exist
	bool RemoteGroup::createAndAddConfirmedTransaction(ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt)
	{
		throw std::runtime_error("not implemented yet");
	}
	void RemoteGroup::addTransactionTriggerEvent(std::shared_ptr<const TransactionTriggerEvent> transactionTriggerEvent)
	{
		throw std::runtime_error("not implemented yet");
	}
	void RemoteGroup::removeTransactionTriggerEvent(const TransactionTriggerEvent& transactionTriggerEvent)
	{
		throw std::runtime_error("not implemented yet");
	}

	bool RemoteGroup::isTransactionExist(ConstGradidoTransactionPtr gradidoTransaction) const
	{
		throw std::runtime_error("not implemented yet");
	}

	//! return events in asc order of targetDate
	std::vector<std::shared_ptr<const TransactionTriggerEvent>> RemoteGroup::findTransactionTriggerEventsInRange(TimepointInterval range)
	{
		throw std::runtime_error("not implemented yet");
	}

	// main search function, do all the work, reference from other functions
	TransactionEntries RemoteGroup::findAll(const Filter& filter /*= Filter::ALL_TRANSACTIONS*/) const
	{
		throw std::runtime_error("not implemented yet");
	}


	std::shared_ptr<const TransactionEntry> RemoteGroup::getTransactionForId(uint64_t transactionId) const
	{
		throw std::runtime_error("not implemented yet");
	}

	//! \param filter use to speed up search if infos exist to narrow down search transactions range
	std::shared_ptr<const TransactionEntry> RemoteGroup::findByMessageId(
		memory::ConstBlockPtr messageId,
		const Filter& filter/* = Filter::ALL_TRANSACTIONS */
	) const
	{
		throw std::runtime_error("not implemented yet");
	}

	AbstractProvider* RemoteGroup::getProvider() const
	{
		throw std::runtime_error("not implemented yet");
	}

	void RemoteGroup::pushTransactionEntry(std::shared_ptr<const gradido::blockchain::TransactionEntry> transactionEntry)
	{
		throw std::runtime_error("not implemented yet");
	}
}