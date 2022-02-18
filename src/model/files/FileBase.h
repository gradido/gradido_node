#ifndef __GRADIDO_NODE_MODEL_FILES_FILE_BASE_H
#define __GRADIDO_NODE_MODEL_FILES_FILE_BASE_H

#include "Poco/Path.h"
#include "Poco/Mutex.h"
#include "Poco/RefCountedObject.h"


namespace model {
	namespace files {
		class FileBase : public Poco::RefCountedObject
		{
		public:
			FileBase(): mRefCount(1) {}
			
		protected:
			//! important, that deconstructor from derived classes is called on release
			virtual ~FileBase() {}
			Poco::FastMutex mFastMutex;
			Poco::FastMutex mReferenceMutex;

			int	 mRefCount;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_FILE_BASE_H