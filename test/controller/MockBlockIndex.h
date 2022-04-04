#ifndef __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_MOCK_H
#define __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_MOCK_H

//#define GTEST_LINKED_AS_SHARED_LIBRARY
//#include "gmock/gmock.h"  // Brings in gMock.
#include "../../src/model/files/BlockIndex.h"
#include "../../src/model/NodeTransactionEntry.h"

namespace controller {
	class MockBlockIndex : public model::files::IBlockIndexReceiver {
	public:

		bool addIndicesForTransaction(
			uint32_t coinColor,
			uint16_t year,
			uint8_t month,
			uint64_t transactionNr,
			int32_t fileCursor,
			const uint32_t* addressIndices,
			uint8_t addressIndiceCount
		) {
			// uint64_t transactionNr, uint8_t month, uint16_t year, uint32_t coinColor, const uint32_t* addressIndices, uint8_t addressIndiceCount
			mTransactionEntrys.push_back(new model::NodeTransactionEntry(
				transactionNr, month, year, coinColor, addressIndices, addressIndiceCount
			));
			return true;
		}

		std::vector < Poco::SharedPtr<model::NodeTransactionEntry>> mTransactionEntrys;
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
