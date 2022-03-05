#ifndef GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H
#define GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H

#include "FileBase.h"
#include "leveldb/db.h"
#include "Poco/Path.h"


/*!
 * @author Dario Rekowski
 * @date 2020-03-03
 * @brief manage group states with leveldb 
 *
 * count on leveldb lib for performance 
*/

namespace model {
	namespace files {
		class State : public FileBase
		{
		public:
			//! open level db file
			State(Poco::Path path);
			//! close leveldb file, should save not already saved key value pairs
			~State();

			//! read value for key from leveldb
			//! \return <not found> if key not found, else value
			std::string getValueForKey(const std::string& key);

			//! read value for key from leveldb as int32 value
			//! \return value or defaultValue if value not found
			int32_t getInt32ValueForKey(const std::string& key, int32_t defaultValue = 0);

			//! put new value key pair into leveldb if key not exist
			//! \return false if key already exist or error occurred, else true
			bool addKeyValue(const std::string& key, const std::string& value);
			//! add new value key pair or update value if key exist
			//! \return true, throw exception on error
			bool setKeyValue(const std::string& key, const std::string& value);

			//! \brief get iterator for looping over every entry
			inline leveldb::Iterator* getIterator() { return mLevelDB->NewIterator(leveldb::ReadOptions()); }

		protected:
			leveldb::DB* mLevelDB;

			//std::unordered_map<std::string, std::string> mDBEntrys;
		};
	}
}

#endif //GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H