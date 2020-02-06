#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H

#include "../model/files/GroupIndex.h"
#include "../lib/DRHashList.h"

#include "Poco/Path.h"

#include "ControllerBase.h"

namespace controller {

	class GroupIndex : public ControllerBase
	{
	public:
		GroupIndex(model::files::GroupIndex* model);
		~GroupIndex();

		size_t update();
		Poco::Path getFolder(const std::string& hash);

	protected:

		struct GroupIndexEntry {
			std::string hash;
			std::string folderName;

			DHASH makeHash() {
				return DRMakeStringHash(hash.data(), hash.size());
			}
		};

		model::files::GroupIndex* mModel;
		DRHashList mHashList;
		std::vector<DHASH> mDoublettes;

		void clear();

	};
};

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H