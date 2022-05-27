#ifndef __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H
#define __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H

#include "CPUTask.h"
#include "../lib/VirtualFile.h"

#include "Poco/Path.h"

namespace task {
	class HddWriteBufferTask: public CPUTask
	{
	public:
		HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, const Poco::Path& path);

		const char* getResourceType() const { return "HddWriteBufferTask"; };
		int run();

	protected:
		std::unique_ptr<VirtualFile> mVirtualFile;
		std::string mFileName;
	};
}

#endif //__GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H