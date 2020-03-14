#ifndef __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H
#define __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H

#include "Poco/Types.h"
#include "Poco/Mutex.h"
#include "Poco/DateTime.h"

#include "../controller/AddressIndex.h"

#include <vector>

namespace model {

	/*!
	* @author Dario Rekowski
	* @date 2020-03-12
	* @brief container for transaction + index details 
	* 
	* Used between multiple controller while write to block is pending and while it is cached
	* It contains the serialized transaction and multiple data for fast indexing
	*/
	class TransactionEntry
	{
	public:
		TransactionEntry()
			: mTransactionNr(0), mFileCursor(-10) {}

		TransactionEntry(uint64_t _transactionNr, std::string _serializedTransaction, Poco::DateTime received, uint16_t addressIndexCount = 2);

		//! \brief init entry object from transaction, deserialize transaction to get infos
		TransactionEntry(std::string _serializedTransaction, uint32_t fileCursor, controller::AddressIndex* addressIndex);

		//! \brief operator for sorting by mTransactionNr in ascending order
		bool operator < (const TransactionEntry& b) { return mTransactionNr < b.mTransactionNr; }

		inline void setFileCursor(uint32_t newFileCursorValue) { Poco::FastMutex::ScopedLock lock(mFastMutex); mFileCursor = newFileCursorValue; }
		inline uint32_t getFileCursor() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mFileCursor; }

		inline void addAddressIndex(uint32_t addressIndex) { Poco::FastMutex::ScopedLock lock(mFastMutex); mAddressIndices.push_back(addressIndex); }
		inline const std::vector<uint32_t>& getAddressIndices() { return mAddressIndices; }

		inline uint64_t getTransactionNr() { return mTransactionNr; }
		inline std::string getSerializedTransaction() { return mSerializedTransaction; }
		inline uint8_t getMonth() { return mMonth; }
		inline uint16_t getYear() { return mYear; }


	protected:
		uint64_t mTransactionNr;
		std::string mSerializedTransaction;
		uint32_t mFileCursor;
		uint8_t mMonth;
		uint16_t mYear;
		Poco::FastMutex mFastMutex;
		std::vector<uint32_t> mAddressIndices;
	};

};

#endif //__GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H
