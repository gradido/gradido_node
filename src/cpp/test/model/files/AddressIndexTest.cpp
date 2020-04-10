#include "AddressIndexTest.h"
#include "../../../lib/BinTextConverter.h"

#include "../../../model/files/AddressIndex.h"
#include "../../../controller/AddressIndex.h"

namespace model {
	namespace files {

		std::string AddressIndexTest::mBinaryAddresses[] = {
			convertHexToBin("94937427d885fe93e22a76a6c839ebc4fdf4e5056012ee088cdebb89a24f778c"),
			convertHexToBin("8190bda585ee5f1d9fbf7d06e81e69ec18e13376104cff54b7457eb7d3ef710d"),
			convertHexToBin("2ed28a1cf5e116d83615406bc577152221c2f774a5656f66a0e7540f7576d71b"),
			convertHexToBin("2ed25e728f162a62b172c6d71f1e118fedca7b25162bcd718a1b2dec1821c721")
		};

		TEST_F(AddressIndexTest, makePath)
		{

			// value 1
			Poco::Path testPath;
			testPath.pushDirectory("pubkeys_94");
			testPath.append("_93.index");

			auto path = controller::AddressIndex::getAddressIndexFilePathForAddress(mBinaryAddresses[0]);
			EXPECT_EQ(path.toString(), testPath.toString());

			// value 2
			testPath.clear();
			testPath.pushDirectory("pubkeys_81");
			testPath.append("_90.index");

			path = controller::AddressIndex::getAddressIndexFilePathForAddress(mBinaryAddresses[1]);
			EXPECT_EQ(path.toString(), testPath.toString());
		}

		TEST_F(AddressIndexTest, addAndReadAddressIndex) 
		{
			Poco::Path path;
			path.pushDirectory(getFilePath());
			path.append(controller::AddressIndex::getAddressIndexFilePathForAddress(mBinaryAddresses[2]));
			removeFile(path);
			AddressIndex addressIndex(path);
			EXPECT_EQ(addressIndex.add(mBinaryAddresses[2], 1), true);
			EXPECT_EQ(addressIndex.add(mBinaryAddresses[2], 1), false);
			EXPECT_EQ(addressIndex.add(mBinaryAddresses[2], 2), false);

			EXPECT_EQ(addressIndex.add(mBinaryAddresses[3], 2), true);

			EXPECT_EQ(addressIndex.getIndexForAddress(mBinaryAddresses[2]), 1);
			EXPECT_EQ(addressIndex.getIndexForAddress(mBinaryAddresses[3]), 2);

			// auto call safeExit on deconstruct addressIndex
		}
	}
}