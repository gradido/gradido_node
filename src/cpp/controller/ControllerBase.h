#ifndef __GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H
#define __GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H

#include "Poco/Mutex.h"

namespace controller {

	class ControllerBase
	{
	public:
	protected:
		Poco::FastMutex mWorkingMutex;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H