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
	bool RemoteGroup::addGradidoTransaction(
		gradido::data::ConstGradidoTransactionPtr gradidoTransaction,
		memory::ConstBlockPtr messageId,
		Timepoint confirmedAt)
	{
		throw std::runtime_error("not implemented yet");
	}

	// main search function, do all the work, reference from other functions
	TransactionEntries RemoteGroup::findAll(const Filter& filter /*= Filter::ALL_TRANSACTIONS*/) const
	{
		throw std::runtime_error("not implemented yet");
	}

	//! find all deferred transfers which have the timeout in date range between start and end, have senderPublicKey and are not redeemed,
	//! therefore boocked back to sender
	//! find all deferred transfers which have the timeout in date range between start and end, have senderPublicKey and are not redeemed,
	//! therefore boocked back to sender
	TransactionEntries RemoteGroup::findTimeoutedDeferredTransfersInRange(
		memory::ConstBlockPtr senderPublicKey,
		TimepointInterval timepointInterval,
		uint64_t maxTransactionNr
	) const
	{
		throw std::runtime_error("not implemented yet");
	}

	//! find all transfers which redeem a deferred transfer in date range
	//! \param senderPublicKey sender public key of sending account of deferred transaction
	//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
	std::list<DeferredRedeemedTransferPair> RemoteGroup::findRedeemedDeferredTransfersInRange(
		memory::ConstBlockPtr senderPublicKey,
		TimepointInterval timepointInterval,
		uint64_t maxTransactionNr
	) const
	{
		throw std::runtime_error("not implemented yet");
	}

	std::shared_ptr<TransactionEntry> RemoteGroup::getTransactionForId(uint64_t transactionId) const
	{
		throw std::runtime_error("not implemented yet");
	}

	//! \param filter use to speed up search if infos exist to narrow down search transactions range
	std::shared_ptr<TransactionEntry> RemoteGroup::findByMessageId(
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

	void RemoteGroup::pushTransactionEntry(std::shared_ptr<gradido::blockchain::TransactionEntry> transactionEntry)
	{
		throw std::runtime_error("not implemented yet");
	}
}