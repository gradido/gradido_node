#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H

#include "gradido_blockchain/blockchain/AbstractProvider.h"
#include "gradido_blockchain/lib/StringViewCompare.h"
#include "FileBased.h"
#include "../controller/GroupIndex.h"

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
			void init(const std::string& communityConfigFile);
			void exit();
		protected:
			void clear();
			std::map<std::string, std::shared_ptr<FileBased>, StringViewCompare> mBlockchainsPerGroup;
			std::mutex mWorkMutex;

		private:
			FileBasedProvider();
			~FileBasedProvider();

			/* Explicitly disallow copying. */
			FileBasedProvider(const FileBasedProvider&) = delete;
			FileBasedProvider& operator= (const FileBasedProvider&) = delete;

			//! load or create blockchain for community, not locking woking mutex!
			void addCommunity(const std::string& alias, const std::string& folder);

			controller::GroupIndex* mGroupIndex;
			bool mInitalized;
		};
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H