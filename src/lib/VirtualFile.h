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
#include <assert.h>

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "Poco/Path.h"

class VirtualFile
{
public:
	//! \brief get memory bin from memory manager by his own
	VirtualFile(size_t bufferSize);

	//! \brief take given memory 
	VirtualFile(unsigned char* buffer, size_t bufferSize);

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
	inline size_t getSize() { return mBufferSize; }

	//! \brief write memory bin content to file, until cursor position
	bool writeToFile(const char* filename);

	bool writeToFile(Poco::Path filePath);

	//! \brief get memory bin in file size and fill it with file content
	//! \return nullptr if file wasn't found or couldn't be locked within 100ms
	static VirtualFile* readFromFile(const char* filename);

protected:
	unsigned char* mBuffer;
	size_t mBufferSize;
	size_t mCursor;

	
};

class BufferOverflowException : public GradidoBlockchainException
{
public:
	explicit BufferOverflowException(const char* what, size_t bufferSize, size_t askSize) noexcept;
	
	std::string getFullString() const;

protected:
	size_t mBufferSize;
	size_t mAskSize;
};

#endif //__GRADIDO_NODE_LIB_VIRTUAL_FILE
