#ifndef __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_TEST_FIXTURE_H
#define __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_TEST_FIXTURE_H

#include "gtest/gtest.h"
#include "../../src/model/NodeTransactionEntry.h"


namespace controller {
	
	class BlockIndexTest : public ::testing::Test
	{
	protected:
		// You can remove any or all of the following functions if their bodies would
		// be empty.

		BlockIndexTest() {
			// You can do set-up work for each test here.
			mTestAddressIndices.push_back({ 2192, 1223, 1234, 3432 });
			mTestAddressIndices.push_back({ 2192, 785, 1234, 121 });
			mTestAddressIndices.push_back({ 17 });
			mTestAddressIndices.push_back({ 32, 17 });

			mTestTransactions.push_back(new model::NodeTransactionEntry(120, 7, 2018, 0xffbb94, mTestAddressIndices[0].data(), 4));
			mTestTransactions.push_back(new model::NodeTransactionEntry(17, 7, 2018, 0xffbb94, mTestAddressIndices[1].data(), 4));
			mTestTransactions.push_back(new model::NodeTransactionEntry(19, 7, 2018, 0xffbb94, mTestAddressIndices[2].data(), 1));
			mTestTransactions.push_back(new model::NodeTransactionEntry(75, 6, 2018, 0xffbb94, mTestAddressIndices[2].data(), 1));
			mTestTransactions.push_back(new model::NodeTransactionEntry(76, 6, 2017, 0xffbb94, mTestAddressIndices[3].data(), 2));

			// add random file cursor positions
			for (auto it = mTestTransactions.begin(); it != mTestTransactions.end(); it++) {
				(*it)->setFileCursor(rand());
			}

		}

		~BlockIndexTest() override {
			// You can do clean-up work that doesn't throw exceptions here.
		}

		// If the constructor and destructor are not enough for setting up
		// and cleaning up each test, you can define the following methods:

		void SetUp() override {
			// Code here will be called immediately after the constructor (right
			// before each test).
		}

		void TearDown() override {
			// Code here will be called immediately after each test (right
			// before the destructor).
		}
	

		// Class members declared here can be used by all tests in the test suite
		// for Foo.
		std::vector<Poco::SharedPtr<model::NodeTransactionEntry>> mTestTransactions;
		std::vector<std::vector<uint32_t>> mTestAddressIndices;
		
	};
}
#endif // __GRADIDO_NODE_TEST_CONTROLLER_BLOCK_INDEX_TEST_FIXTURE_H