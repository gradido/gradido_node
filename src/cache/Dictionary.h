#ifndef __GRADIDO_NODE_CACHE_DICTIONARY_H
#define __GRADIDO_NODE_CACHE_DICTIONARY_H

#include "../model/files/Dictionary.h"

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
	class Dictionary : public TimerCallback
	{
	public:
		Dictionary(std::string_view fileName);
		virtual ~Dictionary();

		//! \return true if address indices could be loaded
		//! \return false if address index file was corrupted
		bool init();
		void exit();
		//! delete dictionary file instance and also file on disk
		void reset();

		//! \brief Get index from cache or if not in cache, loading file, maybe I/O read.
		//! \param address public key as binary string
		//! \return Index or 0 if address didn't exist.
		virtual uint32_t getIndexForString(const std::string& string) const;
		virtual bool hasIndexForString(const std::string& string) const;

		virtual std::string getStringForIndex(uint32_t index) const;		

		//! \brief Get or add index if not exist in cache or file, maybe I/O read.
		//! \param address User public key as binary string
		//! \param lastIndex Last knowing index for group.
		//! \return Index for address.
		virtual uint32_t getOrAddIndexForString(const std::string& string);

		//! \brief Add index, maybe I/O read, I/O write if index is new.
		//! \param address User public key as binary string
		//! \param index Index for address
		//! \return False if index address already exist, else true.
		virtual bool addStringIndex(const std::string& string, uint32_t index);

		inline uint32_t getLastIndex() { std::lock_guard _lock(mFastMutex); return mLastIndex; }

		TimerReturn callFromTimer();

	protected:
		std::string mFileName;
		uint32_t   mLastIndex;
		mutable std::mutex mFastMutex;
		std::shared_ptr<model::files::Dictionary> mDictionaryFile;

		Timepoint mWaitingForNextFileWrite;
	};
}

#endif //__GRADIDO_NODE_CACHE_DICTIONARY_H
