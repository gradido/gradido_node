#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_H

#include "ControllerBase.h"
#include "Poco/Path.h"

namespace controller {

	class Group : public ControllerBase
	{
	public:
		Group(std::string base58Hash, Poco::Path folderPath);

	protected:
		std::string mBase58GroupHash;
		Poco::Path mFolderPath;

	};
}

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_H