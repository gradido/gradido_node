#ifndef __GRADIDO_NODE_CONTROLLER_DEFERRED_TRANSFER_H
#define __GRADIDO_NODE_CONTROLLER_DEFERRED_TRANSFER_H

#include "ControllerBase.h"
#include "../model/files/State.h"

#include <map>

namespace controller
{
	/*
		@author einhornimmond

		@date 05.03.2021

		@brief Cache for active Deferred Transfers

		- free entry if transaction was drained or timeouted on get call
		- store in level db address index as key and transaction nrs as comma separated string as value
	*/

	class DeferredTransfer : public ControllerBase
	{
	public:
		DeferredTransfer(const Poco::Path& groupFolder);
		~DeferredTransfer();

		void addTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);
		std::vector<uint64_t> getTransactionNrsForAddressIndex(uint32_t addressIndex);
		void removeTransactionNrForAddressIndex(uint32_t addressIndex, uint64_t transactionNr);

	protected:
		void updateState(uint32_t addressIndex, std::vector<uint64_t> transactionNrs);
		void loadFromState();

		static std::unique_ptr<std::string> transactionNrsToString(std::vector<uint64_t> transactionNrs);
		static std::vector<uint64_t> transactionNrsToVector(const std::string& transactionNrsString);

		model::files::State mState;
		std::map<uint32_t, std::vector<uint64_t>> mAddressIndexTransactionNrs;

	};
}

#endif //__GRADIDO_NODE_CONTROLLER_DEFERRED_TRANSFER_H