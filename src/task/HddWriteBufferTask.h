#ifndef __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H
#define __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H

#include "CPUTask.h"
#include "../lib/VirtualFile.h"

#include "Poco/Path.h"

namespace task {
	class HddWriteBufferTask: public CPUTask
	{
	public:
		HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, Poco::Path path);

		const char* getResourceType() const { return "HddWriteBufferTask"; };
		int run();

	protected:
		std::unique_ptr<VirtualFile> mVirtualFile;
		Poco::Path mFilePath;
	};
}

#endif //__GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H