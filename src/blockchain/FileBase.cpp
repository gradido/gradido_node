#include "FileBased.h"

namespace gradido {
	namespace blockchain {
		FileBased::FileBased(std::string_view communityId)
			: Abstract(communityId)
		{

		}
		FileBased::~FileBased()
		{

		}
		bool FileBased::addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt)
		{

		}
		TransactionEntries FileBased::findAll(const Filter& filter = Filter::ALL_TRANSACTIONS) const
		{

		}

		TransactionEntries FileBased::findTimeoutedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {

		}

		//! find all transfers which redeem a deferred transfer in date range
		//! \param senderPublicKey sender public key of sending account of deferred transaction
		//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
		std::list<DeferredRedeemedTransferPair> FileBased::findRedeemedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {

		}

		std::shared_ptr<TransactionEntry> FileBased::getTransactionForId(uint64_t transactionId) const
		{

		}
		AbstractProvider* FileBased::getProvider() const
		{

		}

		void FileBased::pushTransactionEntry(std::shared_ptr<TransactionEntry> transactionEntry)
		{

		}
	}
}