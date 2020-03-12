#ifndef __GRADIDO_NODE_MODEL_FILES_FILE_BASE_H
#define __GRADIDO_NODE_MODEL_FILES_FILE_BASE_H

#include "Poco/Path.h"
#include "Poco/Mutex.h"

#include "../../lib/ErrorList.h"

namespace model {
	namespace files {
		class FileBase : public ErrorList
		{
		public:
			FileBase(): mRefCount(1) {}

			//! for poco auto ptr
			void duplicate();
			void release();
		protected:
			Poco::FastMutex mFastMutex;
			Poco::FastMutex mReferenceMutex;

			int	 mRefCount;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_FILE_BASE_H