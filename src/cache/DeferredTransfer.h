#ifndef __GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H
#define __GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H

#include "State.h"
#include "../lib/FuzzyTimer.h"

#include <map>
#include <memory>
#include <set>

namespace cache
{
	/*
		@author einhornimmond

		@date 05.03.2021

		@brief Cache for active Deferred Transfers

		- free entry if transaction was drained on get call
		- store in level db address index as key and transaction nrs as comma separated string as value
		- first transaction nr is deferred transfer themself
		- address index is index of deferred transfer recipient public key
	*/

	class DeferredTransfer : public TimerCallback
	{
	public:
		DeferredTransfer(std::string_view groupFolder, std::string_view communityName);
		~DeferredTransfer();

		bool init(size_t cacheInBytes);
		void exit();
		void reset();

		void addTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);
		std::vector<uint64_t> getTransactionNrsForAddressIndex(uint32_t addressIndex);
		//! check if public key index belong to deferred transfer transaction as recipient
		bool isDeferredTransfer(uint32_t addressIndex);
		void removeTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);
		//! remove address index after it was completly redeemed
		void removeAddressIndex(uint32_t addressIndex);

		//! call f for all deferred transfers
		//! if canBeRemoved return true remove deferred transfer from cache
		void removeAddressIndexes(std::function<bool(uint32_t addressIndex, uint64_t transactionNr)> canBeRemoved);

		virtual TimerReturn callFromTimer();
		virtual const char* getResourceType() const { return "DeferredTransfer"; };

	protected:
		void updateState(uint32_t addressIndex, const std::set<uint64_t>& transactionNrs);
		void loadFromState();

		static std::unique_ptr<std::string> transactionNrsToString(const std::set<uint64_t>& transactionNrs);
		static std::set<uint64_t> transactionNrsToSet(const std::string& transactionNrsString);

		State mState;
		std::map<uint32_t, std::set<uint64_t>> mAddressIndexTransactionNrs;
		std::mutex mFastMutex;
		std::string mTimerName;
		std::string mCommunityName;
	};
}

#endif //__GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H
