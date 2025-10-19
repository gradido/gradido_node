#include "FileBasedProvider.h"
#include "../SystemExceptions.h"
#include "../client/JsonRPC.h"
#include "../client/hiero/ConsensusClient.h"
#include "../client/GraphQL.h"
#include "../ServerGlobals.h"
#include "../task/SyncTopicOnStartup.h"

#include "gradido_blockchain/data/hiero/TopicId.h"

#include "loguru/loguru.hpp"

#include <random>

namespace gradido {
	namespace blockchain {
		FileBasedProvider::FileBasedProvider()
			:mGroupIndex(nullptr), mCommunityIdIndex(ServerGlobals::g_FilesPath + "/communityIdsCache"), mInitalized(false)
		{

		}

		FileBasedProvider::~FileBasedProvider()
		{
			std::lock_guard _lock(mWorkMutex);
			if (mGroupIndex) {
				delete mGroupIndex;
				mGroupIndex = nullptr;
			}
		}

		

		FileBasedProvider* FileBasedProvider::getInstance()
		{
			static FileBasedProvider one;
			return &one;
		}

		std::shared_ptr<Abstract> FileBasedProvider::findBlockchain(std::string_view communityId)
		{
			std::lock_guard _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			auto it = mBlockchainsPerGroup.find(communityId);
			if (it != mBlockchainsPerGroup.end()) {
				return it->second;
			}
			// go manual
			try {
				const auto& groupIndexEntry = mGroupIndex->getCommunityDetails(hiero::TopicId(std::string(communityId)));
				auto it = mBlockchainsPerGroup.find(groupIndexEntry.communityId);
				if (it != mBlockchainsPerGroup.end()) {
					return it->second;
				}
			}
			catch (GradidoBlockchainException& ex) {
				LOG_F(WARNING, "%s", ex.getFullString().data());
			}

			return nullptr;
		}
		bool FileBasedProvider::init(
			const std::string& communityConfigFile,
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients,
			uint8_t hieroClientsPerCommunity/* = 3 */
		) {
			std::lock_guard _lock(mWorkMutex);
			mInitalized = true;
			bool resetAllCommunityIndices = false;
			mHieroClientsPerCommunity = hieroClientsPerCommunity;
			mHieroClients = std::move(hieroClients);
			if (mHieroClientsPerCommunity > mHieroClients.size()) {
				LOG_F(ERROR, "more hiero clients per community as hiero clients");
				return false;
			}
			if (!mCommunityIdIndex.init(GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_ID_INDEX_CACHE_SIZE_MBYTE * 1024 * 1024)) {
				mCommunityIdIndex.reset();
				resetAllCommunityIndices = true;
			}
			mGroupIndex = new cache::GroupIndex(communityConfigFile);
			mGroupIndex->update();
			auto communitiesIds = mGroupIndex->listCommunitiesIds();

			// step 1: check existing blockchain data, one after another
			for (auto& communityId : communitiesIds) {
				// exit if at least one blockchain from config couldn't be loaded
				// should only occure with invalid config
				const auto& details = mGroupIndex->getCommunityDetails(communityId);
				if (!addCommunity(communityId, hiero::TopicId(details.topicId), details.alias, resetAllCommunityIndices)) {
					LOG_F(ERROR, "error adding community %s in folder: %s", details.alias.data(), details.folderName.data());
					return false;
				}
			}
			// step 2: check for new transactions in hiero network, all blockchains at the same time
			for (const auto& pair : mBlockchainsPerGroup) {
				auto task = pair.second->initOnline();
				auto hieroClient = pair.second->pickHieroClient();
				hieroClient->getTopicInfo(pair.second->getHieroTopicId(), task);
				task->scheduleTask(task);
			}

			return true;
		}
		void FileBasedProvider::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mInitalized = false;
			for (auto blockchain : mBlockchainsPerGroup) {
				blockchain.second->exit();
			}
			mBlockchainsPerGroup.clear();
			mCommunityIdIndex.exit();
		}

		int FileBasedProvider::reloadConfig()
		{
			std::lock_guard _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			mGroupIndex->update();
			auto communitiesIds = mGroupIndex->listCommunitiesIds();
			int addedBlockchainsCount = 0;
			for (auto& communityId : communitiesIds) {
				const auto& details = mGroupIndex->getCommunityDetails(communityId);
				auto it = mBlockchainsPerGroup.find(communityId);
				if (it == mBlockchainsPerGroup.end()) {
					if(addCommunity(communityId, hiero::TopicId(details.topicId), details.alias, false)) {
						addedBlockchainsCount++;
					}
				}
				else {
					updateListenerCommunity(communityId, details.alias, it->second);
				}
			}
			return addedBlockchainsCount;
		}

		std::shared_ptr<FileBased> FileBasedProvider::addCommunity(
			const std::string& communityId, 
			const hiero::TopicId& topicId,
			const std::string& alias, 
			bool resetIndices
		) {
			try {
				auto folder = mGroupIndex->getFolder(communityId);

				// with more hiero clients as per community needed, we make sure we not take always the first mHieroClientsPerCommunity from them
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>> hieroClients = mHieroClients; // copy
				if (hieroClients.size() > mHieroClientsPerCommunity) {
					std::shuffle(hieroClients.begin(), hieroClients.end(), std::mt19937{ std::random_device{}() });
					hieroClients.resize(mHieroClientsPerCommunity);
				}

				// with that call community will be initialized and start listening
				auto blockchain = FileBased::create(communityId, topicId, alias, folder, std::move(hieroClients));
				updateListenerCommunity(communityId, alias, blockchain);
				// need to have blockchain in map for init able to work
				mBlockchainsPerGroup.insert({ communityId, blockchain });
				if (!blockchain->init(false)) {
					LOG_F(ERROR, "error initalizing blockchain: %s", communityId.data());
					mBlockchainsPerGroup.erase(communityId);
					return nullptr;
				}
				mCommunityIdIndex.getOrAddIndexForString(communityId);
				return blockchain;
			}
			catch (GradidoBlockchainException& ex) {
				LOG_F(ERROR, "gradido blockchain exception: \n%s\nin community : %s",
					ex.getFullString().data(),
					alias.data()
				);
				return nullptr;
			}
		}
		void FileBasedProvider::updateListenerCommunity(const std::string& communityId, const std::string& alias, std::shared_ptr<FileBased> blockchain)
		{
			const auto& communityConfig = mGroupIndex->getCommunityDetails(communityId);
			// for notification of community server by new transaction
			// deprecated, will be replaced with mqtt in future
			if (!communityConfig.newBlockUri.empty()) {
				std::shared_ptr<client::Base> clientBase;
				auto uri = std::string(communityConfig.newBlockUri);
				if (communityConfig.blockUriType == "json") {
					clientBase = std::make_shared<client::JsonRPC>(uri);
				}
				else if (communityConfig.blockUriType == "graphql") {
					clientBase = std::make_shared<client::GraphQL>(uri);
				}
				else {
					LOG_F(ERROR, "unknown new block uri type: %s", communityConfig.blockUriType.data());
					return;
				}
				if (clientBase) {
					clientBase->setGroupAlias(alias);
					blockchain->setListeningCommunityServer(clientBase);
					LOG_F(INFO, "notification of community: %s", alias.data());
				}
			}
		}
	}
}