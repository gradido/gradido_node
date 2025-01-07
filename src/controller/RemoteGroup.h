#ifndef __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H

#include "gradido_blockchain/blockchain/Abstract.h"

namespace controller {

/*!
  @author Dario Rekowski
 
  @date 20.02.2022
 
  @brief Class for accessing blockchain with this server don't listen, it will be call the transaction from another Gradido Node Server which is listening to this group 
 
 */
	class RemoteGroup : public gradido::blockchain::Abstract
	{
	public:
		RemoteGroup(const std::string& groupAlias);

		//! validate and generate confirmed transaction
		//! throw if gradido transaction isn't valid
		//! \return false if transaction already exist
		virtual bool createAndAddConfirmedTransaction(
			gradido::data::ConstGradidoTransactionPtr gradidoTransaction,
			memory::ConstBlockPtr messageId, 
			Timepoint confirmedAt
		);
		virtual void addTransactionTriggerEvent(std::shared_ptr<const gradido::data::TransactionTriggerEvent> transactionTriggerEvent);
		virtual void removeTransactionTriggerEvent(const gradido::data::TransactionTriggerEvent& transactionTriggerEvent);

		virtual bool isTransactionExist(gradido::data::ConstGradidoTransactionPtr gradidoTransaction) const;

		//! return events in asc order of targetDate
		virtual std::vector<std::shared_ptr<const gradido::data::TransactionTriggerEvent>> findTransactionTriggerEventsInRange(TimepointInterval range);
		// main search function, do all the work, reference from other functions
		virtual gradido::blockchain::TransactionEntries findAll(const gradido::blockchain::Filter& filter = gradido::blockchain::Filter::ALL_TRANSACTIONS) const;
		
		virtual std::shared_ptr<const gradido::blockchain::TransactionEntry> getTransactionForId(uint64_t transactionId) const;

		//! \param filter use to speed up search if infos exist to narrow down search transactions range
		virtual std::shared_ptr<const gradido::blockchain::TransactionEntry> findByMessageId(
			memory::ConstBlockPtr messageId,
			const gradido::blockchain::Filter& filter = gradido::blockchain::Filter::ALL_TRANSACTIONS
		) const;

		virtual gradido::blockchain::AbstractProvider* getProvider() const;

	protected:
		virtual void pushTransactionEntry(std::shared_ptr<const gradido::blockchain::TransactionEntry> transactionEntry);
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_REMOTE_GROUP_H