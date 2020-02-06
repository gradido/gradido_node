#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H

#include "Poco/AutoPtr.h"
#include "Poco/Mutex.h"


#include "../../lib/ErrorList.h"



namespace model {
	class TransactionBase : public ErrorList
	{
	public:
		virtual bool validate() = 0;

	protected:
		Poco::Mutex mWorkingMutex;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BASE_H