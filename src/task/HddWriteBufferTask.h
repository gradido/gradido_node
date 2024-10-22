#ifndef __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H
#define __GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H

#include "CPUTask.h"
#include "../lib/VirtualFile.h"

namespace task {
	class HddWriteBufferTask: public CPUTask
	{
	public:
		HddWriteBufferTask(std::unique_ptr<VirtualFile> vFile, const std::string& path);

		const char* getResourceType() const { return "HddWriteBufferTask"; };
		int run();

	protected:
		std::unique_ptr<VirtualFile> mVirtualFile;
		std::string mFilePath;
	};
}

#endif //__GRADIDO_NODE_TASK_HDD_WRITE_BUFFER_TASK_H