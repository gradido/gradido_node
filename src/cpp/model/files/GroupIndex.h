#ifndef __GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H


#include "Poco/Util/LayeredConfiguration.h"

namespace model {
	namespace files {
		class GroupIndex
		{
		public:
			GroupIndex(const std::string& path);
			inline Poco::AutoPtr<Poco::Util::LayeredConfiguration> getConfig() { return mConfig; }

		protected:
			Poco::AutoPtr<Poco::Util::LayeredConfiguration> mConfig;

		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H