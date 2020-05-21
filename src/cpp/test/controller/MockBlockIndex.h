#ifndef __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_MOCK_H
#define __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_MOCK_H

//#define GTEST_LINKED_AS_SHARED_LIBRARY
#include "gmock/gmock.h"  // Brings in gMock.
#include "../../model/files/BlockIndex.h"

namespace controller {
	class MockBlockIndex : public model::files::IBlockIndexReceiver {
	public: 

		//bool addIndicesForTransaction(Poco::SharedPtr<model::TransactionEntry> transactionEntry);
		//MOCK_METHOD(bool, addIndicesForTransaction, (Poco::SharedPtr<model::TransactionEntry> transactionEntry), ());
		bool addIndicesForTransaction(Poco::SharedPtr<model::TransactionEntry> transactionEntry) { mTransactionEntrys.push_back(transactionEntry); return true; }

		std::vector < Poco::SharedPtr<model::TransactionEntry>> mTransactionEntrys;
	protected: 
		

	};
}

/*class MockTurtle : public Turtle {
public:
	
	MOCK_METHOD(void, PenUp, (), (override));
	MOCK_METHOD(void, PenDown, (), (override));
	MOCK_METHOD(void, Forward, (int distance), (override));
	MOCK_METHOD(void, Turn, (int degrees), (override));
	MOCK_METHOD(void, GoTo, (int x, int y), (override));
	MOCK_METHOD(int, GetX, (), (const, override));
	MOCK_METHOD(int, GetY, (), (const, override));
};
*/

#endif //__GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_MOCK_H