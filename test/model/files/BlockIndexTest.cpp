#include "BlockIndexTest.h"

#include "Poco/File.h"
#include "../../../src/model/files/BlockIndex.h"
#include "../../controller/MockBlockIndex.h"
#include "../../../src/model/NodeTransactionEntry.h"

#include "../../TransactionsBase64.h"

namespace model {
	namespace files {

		TEST_F(ModelFilesTestFixture, ReadAndWrite) {

			// clear block file at first, because appending
			removeFile(std::string("blockTest/blk00000001.index"));

			controller::MockBlockIndex blockIndexController;
			auto blockIndexModelFile = new model::files::BlockIndex("blockTest/", 1);
			blockIndexModelFile->addYearBlock(2020);
			blockIndexModelFile->addMonthBlock(1);
			std::vector<uint32_t> addressIndices1 = { 2192, 1223, 1234, 3432 };
			std::vector<uint32_t> addressIndices2 = { 2192, 785, 1234, 121 };

			blockIndexModelFile->addDataBlock(1210, 0, 0xffaa01, addressIndices1);
			blockIndexModelFile->addMonthBlock(7);
			blockIndexModelFile->addDataBlock(119, 128, 0xccab02, addressIndices2);
			EXPECT_EQ(blockIndexModelFile->writeToFile(), true);

			EXPECT_EQ(blockIndexModelFile->readFromFile(&blockIndexController), true);
			auto transaction1 = blockIndexController.mTransactionEntrys[0];

			EXPECT_EQ(transaction1->getYear(), 2020);
			EXPECT_EQ(transaction1->getMonth(), 1);
			EXPECT_EQ(transaction1->getTransactionNr(), 1210);
			auto transaction1_indices = transaction1->getAddressIndices();
			EXPECT_EQ(transaction1_indices[0], 2192);
			EXPECT_EQ(transaction1_indices[1], 1223);
			EXPECT_EQ(transaction1_indices[2], 1234);
			EXPECT_EQ(transaction1_indices[3], 3432);

			auto transaction2 = blockIndexController.mTransactionEntrys[1];

			EXPECT_EQ(transaction2->getYear(), 2020);
			EXPECT_EQ(transaction2->getMonth(), 7);
			EXPECT_EQ(transaction2->getTransactionNr(), 119);
			auto transaction2_indices = transaction2->getAddressIndices();
			EXPECT_EQ(transaction2_indices[0], 2192);
			EXPECT_EQ(transaction2_indices[1], 785);
			EXPECT_EQ(transaction2_indices[2], 1234);
			EXPECT_EQ(transaction2_indices[3], 121);

			delete blockIndexModelFile;

		}

		TEST_F(ModelFilesTestFixture, InvalidFile) {
			auto blockIndexModelFile = new model::files::BlockIndex("blockTest/", 6);
			controller::MockBlockIndex blockIndexController;
			EXPECT_FALSE(blockIndexModelFile->readFromFile(&blockIndexController));
			delete blockIndexModelFile;
		}
	}
}
