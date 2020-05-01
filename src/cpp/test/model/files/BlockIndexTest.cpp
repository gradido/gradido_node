#include "BlockIndexTest.h"

#include "Poco/File.h"
#include "../../../model/files/BlockIndex.h"

#include "../../TransactionsBase64.h"

namespace model {
	namespace files {

		TEST_F(BlockIndexTest, ReadAndWrite) {

			// clear block file at first, because appending
			removeFile(std::string("blockTest/blk00000001.index"));
			
			BlockIndex blockIndex("blockTest/", 1);
			blockIndex.addYearBlock(2020);
			blockIndex.addMonthBlock(1);
			std::vector<uint32_t> addressIndices1 = { 2192, 1223, 1234, 3432 };
			std::vector<uint32_t> addressIndices2 = { 2192, 785, 1234, 121 };

			blockIndex.addDataBlock(1210, addressIndices1);
			blockIndex.addMonthBlock(7);
			blockIndex.addDataBlock(119, addressIndices2);
			blockIndex.writeToFile();
		}
	}
}