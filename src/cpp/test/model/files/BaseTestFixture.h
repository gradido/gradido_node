#ifndef __GRADIDO_NODE_TEST_MODEL_FILES_BASE_TEST_FIXTURE_H
#define __GRADIDO_NODE_TEST_MODEL_FILES_BASE_TEST_FIXTURE_H

#include "gtest/gtest.h"
#include "../../../model/files/Block.h"
#include "Poco/File.h"

namespace model {
	namespace files {
		class BaseTestFixture : public ::testing::Test
		{
		protected:
			// You can remove any or all of the following functions if their bodies would
			// be empty.

			BaseTestFixture() {
				// You can do set-up work for each test here.
			}

			~BaseTestFixture() override {
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

			void removeFile(const std::string& filename) {
				try {
					std::string path(getFilePath());
					path += "/";
					path += filename;
					//printf("remove %s\n", path.data());
					Poco::File blockFile(path);
					blockFile.remove(false);
				}
				catch (...) {}
			}

			void removeFile(Poco::Path completePath) {
				try {
					Poco::File file(completePath);
					file.remove(false);
				}
				catch (...) {}
			}

			const char* getFilePath() { return "blockTest"; }

			// Class members declared here can be used by all tests in the test suite
			// for Foo.
		};
	}
}
#endif __GRADIDO_NODE_TEST_MODEL_FILES_BASE_TEST_FIXTURE_H