#ifndef GRADIDO_NODE_MODEL_FILES_LEVEL_DB_WRAPPER_H
#define GRADIDO_NODE_MODEL_FILES_LEVEL_DB_WRAPPER_H

#include "leveldb/db.h"

#include <mutex>
#include <functional>

/*!
 * @author Dario Rekowski
 * @date 2020-03-03
 * @brief manage blockchain states with leveldb 
 *
 * count on leveldb lib for performance 
*/

namespace model {
	namespace files {
		class LevelDBWrapper
		{
		public:
			//! open level db file
			LevelDBWrapper(std::string_view folderName);
			//! close leveldb file, should save not already saved key value pairs
			~LevelDBWrapper();

			//! \param cacheInByte use cache for leveldb sepcified in Byte
			bool init(size_t cacheInByte = 0);
			void exit();
			//! remove level db folder, for example if level db was corrupted
			void reset();

			//! read value for key from leveldb
			//! \param value pointer to string in which result will be write
			//! \return true if value could be found
			bool getValueForKey(const char* key, std::string* value);

			//! add new value key pair or update value if key exist
			//! \return true, throw exception on error
			void setKeyValue(const char* key, const std::string& value);

			void removeKey(const char* key);

			//! go through all entries and call callback for each with key, value
			//! don't fill level db cache, verify checksum
			void iterate(std::function<void(leveldb::Slice key, leveldb::Slice value)> callback);

			inline std::string_view getFolderName() const { return mFolderName; }

		protected:
			std::string mFolderName;
			leveldb::Options mOptions;
			leveldb::DB* mLevelDB;
		};
	}
}

#endif //GRADIDO_NODE_MODEL_FILES_LEVEL_DB_WRAPPER_H