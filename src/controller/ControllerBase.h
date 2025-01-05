#ifndef __GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H
#define __GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H

#include <mutex>

namespace controller {

	class ControllerBase
	{
	public:
	protected:
		//! TODO think about needing recursive mutex
		std::mutex mWorkingMutex;
	};
}

#endif //__GRADIDO_NODE_CONTROLLER_CONTROLLER_BASE_H