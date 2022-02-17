/*!
*
* \author Dario Rekowski
*
* \date 20-05-01
*
* \brief VirtualFile for reading and writing files in memory
*
* Load complete File content into memory
* TODO: Loading large files not all at once
*/

#ifndef __GRADIDO_NODE_LIB_VIRTUAL_FILE
#define __GRADIDO_NODE_LIB_VIRTUAL_FILE

#include "../SingletonManager/MemoryManager.h"
#include <assert.h>

class VirtualFile
{
public:
	//! \brief get memory bin from memory manager by his own
	VirtualFile(size_t size);

	//! \brief take given memory bin 
	VirtualFile(MemoryBin* buffer);

	//! \brief handle memory bin back to memory manager
	~VirtualFile();

	//! \brief copy from buffer over into dst, move cursor 
	//! \return false if size + cursor is greater than buffer
	bool read(void* dst, size_t size);

	//! \brief copy to buffer from src, move cursor
	//! \return false if size + cursor is greater than buffer
	bool write(const void* src, size_t size);

	//! \brief set cursor to new dst 
	//! \return false if dst is greater than buffer
	bool setCursor(size_t dst);

	inline size_t getCursor() { return mCursor; }
	inline size_t getSize() { return mBuffer->size(); }

	//! \brief write memory bin content to file, until cursor position
	bool writeToFile(const char* filename);

	//! \brief get memory bin in file size and fill it with file content
	//! \return nullptr if file wasn't found or couldn't be locked within 100ms
	static VirtualFile* readFromFile(const char* filename);

protected:
	MemoryBin* mBuffer;
	size_t mCursor;

	
};

#endif //__GRADIDO_NODE_LIB_VIRTUAL_FILE
