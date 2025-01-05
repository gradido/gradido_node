#include "TransactionTriggerEvent.h"
#include "gradido_blockchain/data/TransactionTriggerEvent.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"

#include "loguru/loguru.hpp"
#include <sstream>

using namespace gradido;
using namespace interaction;
using namespace leveldb;

namespace cache {

	bool TransactionTriggerEvent::init(size_t cacheInBytes)
	{
		auto result = State::init(cacheInBytes);
		if (!result) return result;
		readAllStates([this, &result](Slice key, Slice value) -> void
		{
			std::string_view valueView(value.data(), value.size());
			size_t start = 0;

			// Loop through each Base64-encoded value segment (separated by commas)
			while (start < valueView.size()) {
				// Find the next comma, if there is one
				size_t end = valueView.find(',', start);
				if (end == std::string_view::npos) {
					// If no comma is found, process the rest of the string
					end = valueView.size();
				}
				// decode from base64 to raw data
				auto raw = std::make_shared<memory::Block>(Block::fromBase64(&value.data()[start], end - start));
				// deserialize with help of protopuf to TransactionTriggerEvent
				deserialize::Context deserializer(raw, deserialize::Type::TRANSACTION_TRIGGER_EVENT);
				deserializer.run();
				if (!deserializer.isTransactionTriggerEvent()) {
					LOG_F(ERROR, "error loading transactionTriggerEvent from level db: key: %s, value: %s", key.data(), value.data());
					result = false;
					return;
				}

				auto transactionTriggerEvent = std::make_shared<data::TransactionTriggerEvent>(deserializer.getTransactionTriggerEvent());
				// Insert the TransactionTriggerEvent into the map, using its target date as the key
				mTransactionTriggerEvents.insert({ transactionTriggerEvent->getTargetDate(), transactionTriggerEvent });

				// Move the start point for the next segment after the comma
				start = end + 1;
			}
		});
		return result;
	}

	void TransactionTriggerEvent::reset()
	{
		mTransactionTriggerEvents.clear();
		State::reset();
	}
	
	void TransactionTriggerEvent::addTransactionTriggerEvent(std::shared_ptr<const data::TransactionTriggerEvent> transactionTriggerEvent)
	{
		mTransactionTriggerEvents.insert({ transactionTriggerEvent->getTargetDate(), transactionTriggerEvent });
		updateState(transactionTriggerEvent->getTargetDate());
	}

	void TransactionTriggerEvent::removeTransactionTriggerEvent(const data::TransactionTriggerEvent& transactionTriggerEvent)
	{
		auto range = mTransactionTriggerEvents.equal_range(transactionTriggerEvent.getTargetDate());
		for (auto& it = range.first; it != range.second; it++) {
			if (transactionTriggerEvent.isTheSame(it->second)) {
				mTransactionTriggerEvents.erase(it);
				updateState(transactionTriggerEvent.getTargetDate());
				return;
			}
		}		
		LOG_F(WARNING, "couldn't find transactionTriggerEvent for removal for transaction: %llu", transactionTriggerEvent.getLinkedTransactionId());
	}

	std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> TransactionTriggerEvent::findTransactionTriggerEventsInRange(TimepointInterval range)
	{
		auto startIt = mTransactionTriggerEvents.lower_bound(range.getStartDate());
		auto endIt = mTransactionTriggerEvents.upper_bound(range.getEndDate());
		std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> result;
		result.reserve(std::distance(startIt, endIt));
		for (auto& it = startIt; it != endIt; it++) {
			result.push_back(it->second);
		}
		return result;
	}

	void TransactionTriggerEvent::updateState(gradido::data::TimestampSeconds targetDate)
	{
		auto range = mTransactionTriggerEvents.equal_range(targetDate);
		auto key = std::to_string(targetDate.getSeconds());
		if (!std::distance(range.first, range.second)) {
			removeState(key.data());
			return;
		}
		std::stringstream valueStringStream;
		for (auto& it = range.first; it != range.second; it++) {
			if (it != range.first) {
				valueStringStream << ",";
			}
			valueStringStream << serialize::Context(*it->second).run()->convertToBase64();
		}
		
		State::updateState(key.data(), valueStringStream.str());
	}

}