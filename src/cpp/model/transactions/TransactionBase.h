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

	class GradidoBlock;

	class TransactionBase : public ErrorList
	{
	public:
		TransactionBase();
		TransactionBase(controller::Group* parent);

		virtual bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE) = 0;

		inline void addGroupAlias(const std::string& groupAlias) { mGroupAliases.push_back(groupAlias); }
		void addGroupAliases(TransactionBase* parent);

		virtual void setGroupRoot(Poco::SharedPtr<controller::Group> parent);
		virtual std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) = 0;

		// for poco auto ptr
		void duplicate();
		void release();
	protected:

		std::list<controller::Group*> getGroups();
		void collectGroups(std::vector<std::string> groupAliases, std::list<controller::Group*>& container);

		Poco::Mutex mWorkingMutex;
		Poco::Mutex mAutoPtrMutex;

		Poco::SharedPtr<controller::Group> mGroupRoot;
		std::vector<std::string> mGroupAliases;

		int mReferenceCount;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H
