/*!
*
* \author: einhornimmond
*
* \date: 01.11.19
*
* \brief: manage memory blocks to reduce dynamic memory allocation for preventing memory fragmentation,
* specially for key memory blocks of 32 and 64 byte 
*/

#ifndef GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_MEMORY_MANAGER_H
#define GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_MEMORY_MANAGER_H

#include "Poco/Mutex.h"
#include "../lib/MultithreadContainer.h"

#include <list>
#include <stack>

#define MEMORY_MANAGER_PAGE_SIZE 10

class MemoryPageStack;
class MemoryManager;

class MemoryBin 
{
	friend MemoryPageStack;
	friend MemoryManager;
public: 
	
	inline operator unsigned char*() { return mData; }
	inline operator char*() { return (char*)mData; }
	inline operator void* () { return mData; }
	inline operator const unsigned char*() const { return mData; }
	inline operator const char*() const { return (const char*)mData; }
	
	inline void* operator () (size_t index) { return &mData[index]; }
	inline const void* operator () (size_t index) const { return &mData[index]; }

	inline size_t size() const { return static_cast<size_t>(mSize); }
	inline operator size_t() const { return static_cast<size_t>(mSize); }

	unsigned char* data() { return mData; }
	const unsigned char* data() const { return mData; }

protected:
	MemoryBin(Poco::UInt32 size);
	~MemoryBin();

	Poco::UInt16 mSize;
	unsigned char* mData;

};

class MemoryPageStack : protected UniLib::lib::MultithreadContainer
{
public:
	MemoryPageStack(Poco::UInt16 size);
	~MemoryPageStack();

	MemoryBin* getFreeMemory();
	void releaseMemory(MemoryBin* memory);

protected:
	std::stack<MemoryBin*> mMemoryBinStack;
	Poco::UInt16 mSize;
};

class MemoryManager
{
public:
	~MemoryManager();

	static MemoryManager* getInstance();

	MemoryBin* getFreeMemory(Poco::UInt32 size);
	void releaseMemory(MemoryBin* memory);
	
protected:
	Poco::Int8 getMemoryStackIndex(Poco::UInt16 size);

	MemoryManager();
	MemoryPageStack* mMemoryPageStacks[7];
	bool			 mInitalized;
};



#endif //GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_MEMORY_MANAGER_H