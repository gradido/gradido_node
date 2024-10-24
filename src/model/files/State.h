#ifndef GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H
#define GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H

#include "leveldb/db.h"

#include <mutex>


/*!
 * @author Dario Rekowski
 * @date 2020-03-03
 * @brief manage blockchain states with leveldb 
 *
 * count on leveldb lib for performance 
*/

namespace model {
	namespace files {
		class State
		{
		public:
			//! open level db file
			State(std::string_view folderName);
			//! close leveldb file, should save not already saved key value pairs
			~State();

			bool init();
			void exit();
			//! remove level db folder, for example if level db was corrupted
			void reset();

			//! read value for key from leveldb
			//! \param value pointer to string in which result will be write
			//! \return true if value could be found
			bool getValueForKey(const char* key, std::string* value);

			//! add new value key pair or update value if key exist
			//! \return true, throw exception on error
			bool setKeyValue(const char* key, const std::string& value);

			void removeKey(const char* key);

			//! \brief get iterator for looping over every entry
			inline leveldb::Iterator* getIterator() { return mLevelDB->NewIterator(leveldb::ReadOptions()); }

		protected:
			std::string mFolderName;
			leveldb::DB* mLevelDB;
		};
	}
}

#endif //GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H