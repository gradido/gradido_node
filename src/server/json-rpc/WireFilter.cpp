#include "WireFilter.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/blockchain/CompactFilter.h"
#include "gradido_blockchain/blockchain/PublicKeySearchType.h"
#include "gradido_blockchain/blockchain/SearchDirection.h"
#include "gradido_blockchain/data/compact/PublicKeyIndex.h"
#include "gradido_blockchain/data/TransactionType.h"

using gradido::AppContext;
using gradido::blockchain::CompactFilter, gradido::blockchain::PublicKeySearchType, gradido::blockchain::SearchDirection;
using gradido::data::compact::PublicKeyIndex;
using gradido::data::TransactionType;

namespace server::json_rpc {
  WireFilter::WireFilter()
    : searchDirection(SearchDirection::DESC), transactionType(TransactionType::NONE),
    publicKeySearchType(PublicKeySearchType::None), format(WireOutputFormat::Base64), 
    maxTransactionNr(0), minTransactionNr(0), pagination(20)
  {
  }

  CompactFilter WireFilter::toCompactFilter(AppContext& appContext) const
  {
    CompactFilter resultFilter;
    resultFilter.searchDirection = searchDirection;
    resultFilter.transactionType = transactionType;
    resultFilter.publicKeySearchType = publicKeySearchType;
    if (!coinCommunityId.empty()) {
      resultFilter.coinCommunityIdIndex = appContext.getOrAddCommunityIdIndex(coinCommunityId);
    }
    resultFilter.maxTransactionNr = maxTransactionNr;
    resultFilter.minTransactionNr = minTransactionNr;
    if (!publicKey.isEmpty()) {
      uint32_t communityIdIndex = 0;
      if (!communityId.empty()) {
        communityIdIndex = appContext.getOrAddCommunityIdIndex(communityId);
      }
      else if (resultFilter.coinCommunityIdIndex) {
        communityIdIndex = resultFilter.coinCommunityIdIndex;
      }
      PublicKeyIndex publicKeyIndex{};
      if (communityIdIndex) {
        auto& communityBlockchain = appContext.getCommunityContext(communityIdIndex).getBlockchain();
        if (!communityBlockchain) {
          throw GradidoNullPointerException("missing blockchain for valid community id index", "shared_ptr<Abstract>", __FUNCTION__);
        }
        auto publicKeyIndexSizeT = communityBlockchain->getPublicKeyDictionary().getIndexForData(publicKey);
        if (publicKeyIndexSizeT != (uint32_t)publicKeyIndexSizeT) {
          throw DictionaryOverflowException("to big public key index found", "PublicKey");
        }
        publicKeyIndex = {
          .communityIdIndex = communityIdIndex,
          .publicKeyIndex = (uint32_t)publicKeyIndexSizeT
        };
      }
      if (publicKeyIndex.empty()) {
        // search in all communities for public key
        for (uint32_t i = 1; publicKeyIndex.empty() && appContext.getCommunityIds().hasIndex(i); i++)
        {
          if (i == communityIdIndex) continue;
          auto& communityBlockchain = appContext.getCommunityContext(i).getBlockchain();
          if (!communityBlockchain) {
            throw GradidoNullPointerException("missing blockchain for valid community id index from loop", "shared_ptr<Abstract>", __FUNCTION__);
          }
          auto publicKeyIndexSizeT = communityBlockchain->getPublicKeyDictionary().getIndexForData(publicKey);
          if (publicKeyIndexSizeT != (uint32_t)publicKeyIndexSizeT) {
            throw DictionaryOverflowException("to big public key index found", "PublicKey");
          }
          publicKeyIndex = {
            .communityIdIndex = i,
            .publicKeyIndex = (uint32_t)publicKeyIndexSizeT
          };
        }
      }
      if (!publicKeyIndex.empty()) {
        resultFilter.publicKeyIndex = publicKeyIndex;
      }
    }
    resultFilter.pagination = pagination;
    resultFilter.timepointInterval = timepointInterval;
    return resultFilter;
  }
}