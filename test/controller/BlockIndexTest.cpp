#include "gtest/gtest.h"
#include "../../controller/BlockIndex.h"
#include "BlockIndexTest.h"

namespace controller
{
	TEST_F(BlockIndexTest, AddAndCheck) {
		auto blockIndex = new BlockIndex("blockTest/", 2);
		EXPECT_TRUE(blockIndex->findTransactionsForMonthYear(2018, 7).isNull());
		EXPECT_EQ(blockIndex->findTransactionsForAddressMonthYear(2192, 2018, 7), std::vector<uint64_t>({}));


		for (auto it = mTestTransactions.begin(); it != mTestTransactions.end(); it++) {
			EXPECT_EQ(blockIndex->addIndicesForTransaction(*it), true);
		}
		//std::vector<uint64_t> findTransactionsForAddressMonthYear(uint32_t addressIndex, uint16_t year, uint8_t month);
		EXPECT_EQ(blockIndex->findTransactionsForAddressMonthYear(2192, 2018, 7), std::vector<uint64_t>({ 120,17 }));
		EXPECT_EQ(*blockIndex->findTransactionsForMonthYear(2018, 7), std::vector<uint64_t>({ 120,17,19 }));
		EXPECT_EQ(blockIndex->findTransactionsForAddressMonthYear(17, 2017, 6), std::vector<uint64_t>({ 76 }));
		EXPECT_EQ(*blockIndex->findTransactionsForMonthYear(2017, 6), std::vector<uint64_t>({ 76 }));
		EXPECT_EQ(blockIndex->findTransactionsForAddressMonthYear(785, 2015, 6), std::vector<uint64_t>({}));
		EXPECT_TRUE(blockIndex->findTransactionsForMonthYear(2015, 5).isNull());

		delete blockIndex;
	}

	TEST_F(BlockIndexTest, WriteAndRead) {
		auto blockIndex = new BlockIndex("blockTest/", 2);

		for (auto it = mTestTransactions.begin(); it != mTestTransactions.end(); it++) {
			EXPECT_EQ(blockIndex->addIndicesForTransaction(*it), true);
		}
		EXPECT_TRUE(blockIndex->writeIntoFile());
		delete blockIndex;

		auto blockIndex2 = new BlockIndex("blockTest/", 2);
		ASSERT_TRUE(blockIndex2->loadFromFile());
		

		// same expections like in add and check
		EXPECT_EQ(blockIndex2->findTransactionsForAddressMonthYear(2192, 2018, 7), std::vector<uint64_t>({ 120,17 }));
		EXPECT_EQ(*blockIndex2->findTransactionsForMonthYear(2018, 7), std::vector<uint64_t>({ 120,17,19 }));
		EXPECT_EQ(blockIndex2->findTransactionsForAddressMonthYear(17, 2017, 6), std::vector<uint64_t>({ 76 }));
		EXPECT_EQ(*blockIndex2->findTransactionsForMonthYear(2017, 6), std::vector<uint64_t>({ 76 }));
		EXPECT_EQ(blockIndex2->findTransactionsForAddressMonthYear(785, 2015, 6), std::vector<uint64_t>({}));
		EXPECT_TRUE(blockIndex2->findTransactionsForMonthYear(2015, 5).isNull());

		delete blockIndex2;


	}

	TEST_F(BlockIndexTest, InvalidFile) {
		auto blockIndex = new BlockIndex("blockText/", 2);
		EXPECT_FALSE(blockIndex->loadFromFile());

		delete blockIndex;
	}
}