#ifndef __GRADIDO_NODE_TASK_SERIALIZE_TO_VFILE_TASK_H
#define __GRADIDO_NODE_TASK_SERIALIZE_TO_VFILE_TASK_H

#include "CPUTask.h"
#include "../lib/VirtualFile.h"
#include "Poco/SharedPtr.h"

namespace task {

	class ISerializeToVFile 
	{
	public:
		//! serialize for writing with hdd write buffer task
		virtual std::unique_ptr<VirtualFile> serialize() = 0;
		virtual std::string getFileNameString() = 0;
	};

	/*!
		@author einhornimmond
		@date 10.06.2022
		@brief Task for serializing binary data in preparing for saving in file

		create and schedule HDD write buffer task afterwards
	*/
	class SerializeToVFileTask : public CPUTask
	{
	public: 
		SerializeToVFileTask(Poco::SharedPtr<ISerializeToVFile> dataProvider);
		~SerializeToVFileTask();

		const char* getResourceType() const { return "SerializeToVFileTask"; };
		int run();

	protected:
		Poco::SharedPtr<ISerializeToVFile> mDataProvider;
	};

}

#endif // __GRADIDO_NODE_TASK_SERIALIZE_TO_VFILE_TASK_H