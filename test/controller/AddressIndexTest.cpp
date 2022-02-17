#include "gtest/gtest.h"

#include "../../controller/AddressIndex.h"

TEST(ControllerAddressIndexTest, addAndCheck) 
{
	auto adressIndex = new controller::AddressIndex("blockTest/", 0);



	delete adressIndex;
}