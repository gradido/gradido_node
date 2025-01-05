#ifndef __GRADIDO_NODE_CACHE_DICTIONARY_H
#define __GRADIDO_NODE_CACHE_DICTIONARY_H

#include "../model/files/LevelDBWrapper.h"
#include "gradido_blockchain/crypto/SignatureOctet.h"

#include <unordered_map>

namespace gradido {
	namespace blockchain {
		class FileBased;
	}
}

namespace cache {
	

	/*!
	* @author Dario Rekowski
	* @date 2020-02-06
	*
	* @brief Create Indices for strings, for more efficient working with strings in memory
	* So basically every one use only the uint32_t index value for comparisation and stuff and only if the real value is needed,
	* they will ask the Dictionary for the real value
	*
	*/
	class Dictionary
	{
	public:
		Dictionary(std::string_view fileName);
		virtual ~Dictionary();

		//! \param cacheInBytes level db cache in bytes, 0 for no cache
		//! \return true if address indices could be loaded
		//! \return false if address index file was corrupted
		bool init(size_t cacheInBytes);
		void exit();
		//! delete dictionary file instance and also file on disk
		void reset();

		//! \brief Get index from cache or if not in cache, loading file, maybe I/O read.
		//! \param address public key as binary string
		//! \return Index or throw exception if index not exist
		virtual uint32_t getIndexForString(const std::string& string) const;
		virtual inline bool hasString(const std::string& string) const {
			return findIndexForString(string) != 0;
		}

		virtual std::string getStringForIndex(uint32_t index) const;		
		virtual bool hasIndex(uint32_t index) const;

		//! \brief Get or add index if not exist in cache or file, maybe I/O read.
		//! \param address User public key as binary string
		//! \param lastIndex Last knowing index for group.
		//! \return Index for address.
		virtual uint32_t getOrAddIndexForString(const std::string& string);

		//! \brief Add index
		//! \param address User public key as binary string
		//! \param index Index for address
		//! \return False if index address already exist, else true.
		virtual bool addStringIndex(const std::string& string, uint32_t index);

		inline uint32_t getLastIndex() { std::lock_guard _lock(mFastMutex);  return mLastIndex; }

	protected:
		bool hasStringIndexPair(const std::string& string, uint32_t index) const;
		uint32_t findIndexForString(const std::string& string) const;

		mutable std::mutex mFastMutex;
		uint32_t mLastIndex;
		// key is index, value is string
		mutable model::files::LevelDBWrapper mDictionaryFile;
		std::unordered_map<SignatureOctet, uint32_t> mValueKeyReverseLookup;
	};
}

#endif //__GRADIDO_NODE_CACHE_DICTIONARY_H
