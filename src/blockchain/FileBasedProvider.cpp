#include "FileBasedProvider.h"
#include "../SystemExceptions.h"
#include "../client/JsonRPC.h"
#include "../client/hiero/ConsensusClient.h"
#include "../client/GraphQL.h"
#include "../ServerGlobals.h"
#include "../task/SyncTopicOnStartup.h"

#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/lib/DictionaryExceptions.h"

#include "loguru/loguru.hpp"

#include <memory>
#include <random>
#include <shared_mutex>
#include <string>
#include <vector>

using std::shared_lock, std::unique_lock;
using std::shared_ptr, std::make_shared;
using std::string;
using std::vector;

namespace gradido {
	namespace blockchain {
		FileBasedProvider::FileBasedProvider()
			:mGroupIndex(nullptr), mInitalized(false)
		{

		}

		FileBasedProvider::~FileBasedProvider()
		{
			unique_lock _lock(mWorkMutex);
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

		shared_ptr<Abstract> FileBasedProvider::findBlockchain(uint32_t communityIdIndex)
		{
			shared_lock _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			auto it = mBlockchainsPerGroup.find(communityIdIndex);
			if (it != mBlockchainsPerGroup.end()) {
				return it->second;
			}


			return nullptr;
		}

		shared_ptr<Abstract> FileBasedProvider::findBlockchain(const string& communityId)
		{
			auto communityIdIndex = g_appContext->getCommunityIds().getIndexForData(communityId);
			if (!communityIdIndex.has_value()) {
				LOG_F(WARNING, "no community id index for %s", communityId.c_str());
			}
			else {
				return findBlockchain(communityIdIndex.value());
			}
			return nullptr;
		}

		shared_ptr<Abstract> FileBasedProvider::findBlockchain(hiero::TopicId& topicId)
		{
			try {
				const auto& groupIndexEntry = mGroupIndex->getCommunityDetails(topicId);
				auto communityIdIndex = g_appContext->getCommunityIds().getIndexForData(groupIndexEntry.communityId);
				if (!communityIdIndex.has_value()) {
					LOG_F(WARNING, "no community id index for %s", groupIndexEntry.communityId.c_str());
				}
				else {
					return findBlockchain(communityIdIndex.value());
				}
			}
			catch (GradidoBlockchainException& ex) {
				LOG_F(WARNING, "%s", ex.getFullString().data());
			}
			return nullptr;
		}

		bool FileBasedProvider::init(
			const string& communityConfigFile,
			vector<shared_ptr<client::hiero::ConsensusClient>>&& hieroClients,
			uint8_t hieroClientsPerCommunity/* = 3 */
		) {
			unique_lock _lock(mWorkMutex);
			mInitalized = true;
			bool resetAllCommunityIndices = false;
			mHieroClientsPerCommunity = hieroClientsPerCommunity;
			mHieroClients = std::move(hieroClients);
			if (mHieroClientsPerCommunity > mHieroClients.size()) {
				LOG_F(ERROR, "more hiero clients per community as hiero clients");
				return false;
			}
			mGroupIndex = new cache::GroupIndex(communityConfigFile);
			mGroupIndex->update();
			auto communitiesIds = mGroupIndex->listCommunitiesIds();

			// step 1: check existing blockchain data, one after another
			for (auto& communityId : communitiesIds) {
				// exit if at least one blockchain from config couldn't be loaded
				// should only occure with invalid config
				const auto& details = mGroupIndex->getCommunityDetails(communityId);
				if (!addCommunity(communityId, hiero::TopicId(details.topicId), details.alias)) {
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
			unique_lock _lock(mWorkMutex);
			mInitalized = false;
			for (auto blockchain : mBlockchainsPerGroup) {
				blockchain.second->exit();
			}
			mBlockchainsPerGroup.clear();
		}

		int FileBasedProvider::reloadConfig()
		{
			unique_lock _lock(mWorkMutex);
			if (!mInitalized) {
				throw ClassNotInitalizedException("please call init before", "blockchain::FileBasedProvider");
			}
			mGroupIndex->update();
			int addedBlockchainsCount = 0;
			mGroupIndex->iterate([&](const cache::CommunityIndexEntry& details) -> bool {
				auto it = mBlockchainsPerGroup.find(details.communityIdIndex);
				if (it == mBlockchainsPerGroup.end()) {
					if (addCommunity(details.communityId, hiero::TopicId(details.topicId), details.alias)) {
						addedBlockchainsCount++;
					}
				}
				else {
					updateListenerCommunity(details.communityIdIndex, details.alias, it->second);
				}
				return true;
			});
			return addedBlockchainsCount;
		}

		shared_ptr<FileBased> FileBasedProvider::addCommunity(
			const string& communityId, 
			const hiero::TopicId& topicId,
			const string& alias
		) {
			try {
				auto communityIdIndex = g_appContext->getOrAddCommunityIdIndex(communityId);
				auto folder = mGroupIndex->getFolder(communityIdIndex);

				// with more hiero clients as per community needed, we make sure we not take always the first mHieroClientsPerCommunity from them
				vector<shared_ptr<client::hiero::ConsensusClient>> hieroClients = mHieroClients; // copy
				if (hieroClients.size() > mHieroClientsPerCommunity) {
					std::shuffle(hieroClients.begin(), hieroClients.end(), std::mt19937{ std::random_device{}() });
					hieroClients.resize(mHieroClientsPerCommunity);
				}

				// with that call community will be initialized and start listening
				auto blockchain = FileBased::create(communityId, topicId, alias, folder, std::move(hieroClients));
				updateListenerCommunity(communityIdIndex, alias, blockchain);
				// need to have blockchain in map for init able to work
				mBlockchainsPerGroup.insert({ communityIdIndex, blockchain });
				if (!blockchain->init(false)) {
					LOG_F(ERROR, "error initalizing blockchain: %s", communityId.data());
					mBlockchainsPerGroup.erase(communityIdIndex);
					return nullptr;
				}
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
		void FileBasedProvider::updateListenerCommunity(uint32_t communityIdIndex, const string& alias, shared_ptr<FileBased> blockchain)
		{
			const auto& communityConfig = mGroupIndex->getCommunityDetails(communityIdIndex);
			// for notification of community server by new transaction
			// deprecated, will be replaced with mqtt in future
			if (!communityConfig.newBlockUri.empty()) {
				shared_ptr<client::Base> clientBase;
				auto uri = string(communityConfig.newBlockUri);
				if (communityConfig.blockUriType == "json") {
					clientBase = make_shared<client::JsonRPC>(uri);
				}
				else if (communityConfig.blockUriType == "graphql") {
					clientBase = make_shared<client::GraphQL>(uri);
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