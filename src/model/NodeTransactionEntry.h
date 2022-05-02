#ifndef __GRADIDO_NODE_MODEL_NODE_TRANSACTION_ENTRY_H
#define __GRADIDO_NODE_MODEL_NODE_TRANSACTION_ENTRY_H

#include "Poco/Types.h"
#include "Poco/Mutex.h"
#include "Poco/DateTime.h"

#include "../controller/Group.h"
#include "gradido_blockchain/model/TransactionEntry.h"

#include <vector>

namespace model {

	/*!
	* @author einhornimmond
	* @date 2020-02-27
	* @brief container for transaction + index details
	*
	* Used between multiple controller while write to block is pending and while it is cached
	* It contains the serialized transaction and multiple data for fast indexing
	*/
	class NodeTransactionEntry : public TransactionEntry
	{
	public:
		NodeTransactionEntry()
			: mFileCursor(0) {}

		NodeTransactionEntry(gradido::GradidoBlock* transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex, int32_t fileCursor = -10);

		//! \brief init entry object from details e.g. by loading from file
		NodeTransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, const std::string& coinGroupId, const uint32_t* addressIndices, uint8_t addressIndiceCount);

		inline void setFileCursor(int32_t newFileCursorValue) { Poco::FastMutex::ScopedLock lock(mFastMutex); mFileCursor = newFileCursorValue; }
		inline int32_t getFileCursor() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mFileCursor; }

		inline void addAddressIndex(uint32_t addressIndex) { Poco::FastMutex::ScopedLock lock(mFastMutex); mAddressIndices.push_back(addressIndex); }
		inline std::vector<uint32_t> getAddressIndices() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mAddressIndices; }

	protected:
		int32_t mFileCursor;
		std::vector<uint32_t> mAddressIndices;
	};

};

#endif //__GRADIDO_NODE_MODEL_NODE_TRANSACTION_ENTRY_H
