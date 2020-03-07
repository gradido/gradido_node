#ifndef GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H
#define GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H

#include "FileBase.h"
#include "leveldb/db.h"
#include "Poco/Path.h"

#include <unordered_map>

namespace model {
	namespace files {
		class GroupState : public FileBase
		{
		public:
			GroupState(Poco::Path path);
			~GroupState();

			std::string getValueForKey(const std::string& key);
			int32_t getInt32ValueForKey(const std::string& key, int32_t defaultValue = 0);
			bool addKeyValue(const std::string& key, const std::string& value);
			bool setKeyValue(const std::string& key, const std::string& value);

		protected:
			leveldb::DB* mLevelDB;

			//std::unordered_map<std::string, std::string> mDBEntrys;
		};
	}
}

#endif //GRADIDO_NODE_MODEL_FILES_GROUP_STATE_H