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
		TRANSACTION_VALIDATION_SINGLE = 1,
		// check also with previous transaction
		TRANSACTION_VALIDATION_SINGLE_PREVIOUS = 2,
		// check all transaction from within date range
		// by creation automatic the same month
		TRANSACTION_VALIDATION_DATE_RANGE = 4,
		// check paired transaction on another group by cross group transactions
		TRANSACTION_VALIDATION_PAIRED = 8,
		// check all transactions in the group which connected with this transaction address(es)
		TRANSACTION_VALIDATION_CONNECTED_GROUP = 16,
		// check all transactions which connected with this transaction
		TRANSACTION_VALIDATION_CONNECTED_BLOCKCHAIN = 32
	};

	class GradidoBlock;

	class TransactionBase : public ErrorList
	{
	public:
		TransactionBase();
		TransactionBase(Poco::SharedPtr<controller::Group> groupRoot);

		virtual bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE) = 0;

		inline void addGroupAlias(const std::string& groupAlias) { mGroupAliases.push_back(groupAlias); }
		void addGroupAliases(TransactionBase* parent);

		virtual void setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot);
		virtual void setGradidoBlock(GradidoBlock* gradidoBlock);
		virtual std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) = 0;

		inline Poco::SharedPtr<controller::Group> getGroupRoot() { return mGroupRoot; }

		// for poco auto ptr
		void duplicate();
		void release();
	protected:

		std::list<controller::Group*> getGroups();
		void collectGroups(std::vector<std::string> groupAliases, std::list<controller::Group*>& container);

		Poco::Mutex mWorkingMutex;
		Poco::Mutex mAutoPtrMutex;

		Poco::SharedPtr<controller::Group> mGroupRoot;
		GradidoBlock* mGradidoBlock;
		std::vector<std::string> mGroupAliases;

		int mReferenceCount;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H
