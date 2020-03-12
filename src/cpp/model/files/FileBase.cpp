#include "FileBase.h"

namespace model {
	namespace files {
		void FileBase::duplicate()
		{
			Poco::FastMutex::ScopedLock lock(mReferenceMutex);
			mRefCount++;
		}

		void FileBase::release()
		{
			Poco::FastMutex::ScopedLock lock(mReferenceMutex);
			mRefCount--;
			if (0 == mRefCount) {	
				delete this;
			}
		}
	}
}