#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H

#include "Poco/AutoPtr.h"
#include "Poco/Mutex.h"


#include "../../lib/ErrorList.h"

namespace controller {
	class Group;
	class AddressIndex;
}

namespace model {

	enum TransactionValidationLevel {
		// check only the transaction
		TRANSACTION_VALIDATION_SINGLE,
		// check also with previous transaction
		TRANSACTION_VALIDATION_SINGLE_PREVIOUS,
		// check all transaction from within date range
		// by creation automatic the same month
		TRANSACTION_VALIDATION_DATE_RANGE,
		// check all transactions from this address(es)
		TRANSACTION_VALIDATION_FULL_ADDRESS,
		// check all transactions in the group which connected with this transaction address(es)
		TRANSACTION_VALIDATION_CONNECTED_GROUP,
		// check all transactions in this group
		TRANSACTION_VALIDATION_FULL_GROUP,
		// check all transactions which connected with this transaction
		TRANSACTION_VALIDATION_CONNECTED_BLOCKCHAIN,
		// check every transaction in every group
		TRANSACTION_VALIDATION_FULL_BLOCKCHAIN
	};

	class Transaction;
	
	class TransactionBase : public ErrorList
	{
	public:
		TransactionBase();
		TransactionBase(Transaction* parent);

		virtual bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE) = 0;

		inline void addBase58GroupHash(const std::string& base58GroupHash) { mBase58GroupHashes.push_back(base58GroupHash); }
		void addBase58GroupHashes(TransactionBase* parent);

		virtual void setParent(Transaction* parent);
		virtual std::vector<uint32_t> getInvolvedAddressIndices(controller::AddressIndex* addressIndexContainer) = 0;

		// for poco auto ptr
		void duplicate();
		void release();
	protected:

		std::list<controller::Group*> getGroups();
		void collectGroups(std::vector<std::string> groupBase58Hashes, std::list<controller::Group*>& container);

		Poco::Mutex mWorkingMutex;
		Poco::Mutex mAutoPtrMutex;

		Transaction* mParent;
		std::vector<std::string> mBase58GroupHashes;

		int mReferenceCount;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H