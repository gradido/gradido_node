#ifndef __GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H
#define __GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H

#include "State.h"

#include <map>
#include <memory>

namespace cache
{
	/*
		@author einhornimmond

		@date 05.03.2021

		@brief Cache for active Deferred Transfers

		- free entry if transaction was drained on get call
		- store in level db address index as key and transaction nrs as comma separated string as value
		- first transaction nr is deferred transfer themself
	*/

	class DeferredTransfer
	{
	public:
		DeferredTransfer(std::string_view groupFolder);
		~DeferredTransfer();

		bool init();
		void exit();
		void reset();

		void addTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);
		std::vector<uint64_t> getTransactionNrsForAddressIndex(uint32_t addressIndex);
		void removeTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);

	protected:
		void updateState(uint32_t addressIndex, std::vector<uint64_t> transactionNrs);
		void loadFromState();

		static std::unique_ptr<std::string> transactionNrsToString(std::vector<uint64_t> transactionNrs);
		static std::vector<uint64_t> transactionNrsToVector(const std::string& transactionNrsString);

		State mState;
		std::map<uint32_t, std::vector<uint64_t>> mAddressIndexTransactionNrs;
		std::mutex mFastMutex;
	};
}

#endif //__GRADIDO_NODE_CACHE_DEFERRED_TRANSFER_H
