#include "BlockTest.h"


#include "../../../src/model/files/Block.h"

namespace model {
	namespace files {
		const std::string line1 = "Hallo Welt, Test Line 1";
		const std::string line2 = "Dies ist noch eine Test Line";

		TEST_F(BlockTest, WriteAndReadLines) {
			
			// clear block file at first, because appending
			removeFile(std::string("blk00000001.dat"));
			

			Block b("blockTest/", 1);
			EXPECT_EQ(b.appendLine(&line1), 0);
			EXPECT_EQ(b.validateHash(), true);
			EXPECT_EQ(b.appendLine(&line2), line1.size() + sizeof(Poco::UInt16));
			EXPECT_EQ(b.validateHash(), true);
			EXPECT_EQ(b.getCurrentFileSize(), line1.size() + line2.size() + 2 * sizeof(Poco::UInt16));
			std::string readedLine1;
			EXPECT_NO_THROW(b.readLine(0));
			EXPECT_EQ(readedLine1, line1);
		}

		TEST_F(BlockTest, ReadLines) {
			
			Block b("blockTest/", 1);
			auto readedLine2 = b.readLine(line1.size() + sizeof(Poco::UInt16));
			EXPECT_EQ(readedLine2->compare(line2), 0);
		}

	}
}
