#include "Group.h"

namespace controller {
	Group::Group(std::string base58Hash, Poco::Path folderPath)
		: mBase58GroupHash(base58Hash), mFolderPath(folderPath)
	{

	}

}