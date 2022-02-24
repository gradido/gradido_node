#ifndef __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H
#define __GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H

#include "Poco/Types.h"
#include "Poco/Mutex.h"
#include "Poco/DateTime.h"

#include "../controller/Group.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

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
			: mTransactionNr(0), mFileCursor(0) {}

		//! \brief init entry object from serialized transaction, deserialize transaction to get infos
		TransactionEntry(std::unique_ptr<std::string> _serializedTransaction, int32_t fileCursor, Poco::SharedPtr<controller::Group> groupRoot);

		TransactionEntry(gradido::GradidoBlock* transaction, Poco::SharedPtr<controller::AddressIndex> addressIndex);

		//! \brief init entry object from details e.g. by loading from file
		TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor, const uint32_t* addressIndices, uint8_t addressIndiceCount);

		//! \brief init entry object without indices
		TransactionEntry(uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor);


		//! \brief operator for sorting by mTransactionNr in ascending order
		bool operator < (const TransactionEntry& b) { return mTransactionNr < b.mTransactionNr; }

		inline void setFileCursor(int32_t newFileCursorValue) { Poco::FastMutex::ScopedLock lock(mFastMutex); mFileCursor = newFileCursorValue; }
		inline int32_t getFileCursor() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mFileCursor; }

		inline void addAddressIndex(uint32_t addressIndex) { Poco::FastMutex::ScopedLock lock(mFastMutex); mAddressIndices.push_back(addressIndex); }
		inline std::vector<uint32_t> getAddressIndices() { Poco::FastMutex::ScopedLock lock(mFastMutex); return mAddressIndices; }

		inline uint64_t getTransactionNr() { return mTransactionNr; }
		inline const std::string* getSerializedTransaction() { return mSerializedTransaction.get(); }
		inline uint8_t getMonth() { return mMonth; }
		inline uint16_t getYear() { return mYear; }
		inline uint32_t getCoinColor() { return mCoinColor; }

	protected:
		uint64_t mTransactionNr;
		std::unique_ptr<std::string> mSerializedTransaction;
		int32_t mFileCursor;
		uint8_t mMonth;
		uint16_t mYear;
		uint32_t mCoinColor;
		Poco::FastMutex mFastMutex;
		std::vector<uint32_t> mAddressIndices;
	};

};

#endif //__GRADIDO_NODE_MODEL_TRANSACTION_ENTRY_H
