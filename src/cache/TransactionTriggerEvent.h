#ifndef __GRADIDO_NODE_CACHE_TRANSACTION_TRIGGER_EVENT_H
#define __GRADIDO_NODE_CACHE_TRANSACTION_TRIGGER_EVENT_H

#include "gradido_blockchain/lib/TimepointInterval.h"
#include "gradido_blockchain/data/Timestamp.h"

#include "State.h"

#include <map>

namespace gradido {
	namespace data {
		class TransactionTriggerEvent;
	}
}

namespace cache {
	class TransactionTriggerEvent : protected State
	{
	public:
		using State::State;
		bool init(size_t cacheInBytes);
		using State::exit;
		void reset();

		void addTransactionTriggerEvent(std::shared_ptr<const gradido::data::TransactionTriggerEvent> transactionTriggerEvent);
		void removeTransactionTriggerEvent(const gradido::data::TransactionTriggerEvent& transactionTriggerEvent);
		std::vector<std::shared_ptr<const gradido::data::TransactionTriggerEvent>> findTransactionTriggerEventsInRange(TimepointInterval range);

	protected:
		// store all transaction trigger events, belonging to the same target date as key,value pair in level db
		// key = targetDate, value = base64(serialized(data::TransactionTriggerEvent)), base64(serialized(data::TransactionTriggerEvent))
		void updateState(gradido::data::Timestamp targetDate);
		std::multimap<gradido::data::Timestamp, std::shared_ptr<const gradido::data::TransactionTriggerEvent>> mTransactionTriggerEvents;

	};
}

#endif //__GRADIDO_NODE_CACHE_TRANSACTION_TRIGGER_EVENT_H