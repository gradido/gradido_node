#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H

#include "gradido_blockchain/blockchain/AbstractProvider.h"
#include "gradido_blockchain/lib/StringViewCompare.h"
#include "FileBased.h"

namespace gradido {
	namespace blockchain {
		/*
		* @author einhornimmond
		* @date 18.08.2025
		* @brief singleton, hold FileBasedBlockchains per community topic id
		*/
		class FileBasedProvider : public AbstractProvider
		{
		public:
			static FileBasedProvider* getInstance();

			std::shared_ptr<Abstract> findBlockchain(std::string_view communityId);
			void clear();
		protected:
			std::map<std::string, std::shared_ptr<FileBased>, StringViewCompare> mBlockchainsPerGroup;
			std::mutex mWorkMutex;

		private:
			FileBasedProvider();
			~FileBasedProvider();

			/* Explicitly disallow copying. */
			FileBasedProvider(const FileBasedProvider&) = delete;
			FileBasedProvider& operator= (const FileBasedProvider&) = delete;
		};
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H