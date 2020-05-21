#ifndef __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H
#define __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H

#include "Poco/Types.h"
#include "Poco/Mutex.h"
#include "Poco/DateTime.h"

#include "../controller/AddressIndex.h"
#include "transactions/Transaction.h"

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

		//! \brief init entry object from serialized transaction, deserialize transaction to get infos
		TransactionEntry(std::string _serializedTransaction, uint32_t fileCursor, Poco::SharedPtr<controller::AddressIndex> addressIndex);

		TransactionEntry(Poco::AutoPtr<Transaction> transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex);

		//! \brief init entry object from details e.g. by loading from file
		TransactionEntry(uint64_t transactionNr, uint32_t fileCursor, uint8_t month, uint16_t year, uint32_t* addressIndices, uint8_t addressIndiceCount);
		

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
