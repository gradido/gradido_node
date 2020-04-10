#include "BlockIndexTest.h"

#include "Poco/File.h"
#include "../../../model/files/BlockIndex.h"

#include "../../TransactionsBase64.h"

namespace model {
	namespace files {

		TEST(BlockIndexTest, ReadAndWrite) {

			// clear block file at first, because appending
			try {
				Poco::File blockIndexFile("blockTest/blk00000001.index");
				blockIndexFile.remove(false);
			}
			catch (...) {}
			

			BlockIndex blockIndex("blockTest/", 1);

		}
	}
}