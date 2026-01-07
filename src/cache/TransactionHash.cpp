#include "TransactionHash.h"

#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/NodeTransactionEntry.h"
#include "gradido_blockchain/data/GradidoTransaction.h"
#include "gradido_blockchain/serialization/toJsonString.h"


#include "loguru/loguru.hpp"

using namespace gradido;
using namespace blockchain;
using namespace data;

using serialization::toJsonString;

namespace cache {
	TransactionHash::TransactionHash(std::string_view communityId)
		: //mSignaturePartTransactionNrs(MAGIC_NUMBER_SIGNATURE_CACHE),
		mCommunityId(communityId)
	{

	}

	TransactionHash::~TransactionHash()
	{
		// mSignaturePartTransactionNrs.clear();
		mBodyBytesTransactionNrs.clear();
	}

	void TransactionHash::push(const gradido::data::ConfirmedTransaction& confirmedTransaction)
	{
		SignatureOctet hash = deriveHash(*confirmedTransaction.getGradidoTransaction());
		// auto pair = mSignaturePartTransactionNrs.get(hash);
		auto it = mBodyBytesTransactionNrs.find(confirmedTransaction.getGradidoTransaction()->getBodyBytes());
		//if (pair.has_value()) {
		if (it != mBodyBytesTransactionNrs.end()) {
			auto storedTransaction = FileBasedProvider::getInstance()->findBlockchain(mCommunityId)->getTransactionForId(it->second);
			LOG_F(ERROR, "Hash collision detected, %s and %s have the same hash",
				toJsonString(confirmedTransaction, true).c_str(),
				toJsonString(*storedTransaction->getConfirmedTransaction(), true).c_str()
			);
			throw GradidoAlreadyExist("key already exist");
		}
		//mSignaturePartTransactionNrs.add(hash, confirmedTransaction.getId());
		mBodyBytesTransactionNrs.insert({ confirmedTransaction.getGradidoTransaction()->getBodyBytes(), confirmedTransaction.getId() });
	}

	bool TransactionHash::has(const data::GradidoTransaction& transaction) const
	{
		// get first signature from transaction
		auto hash = deriveHash(transaction);
		// auto pair = mSignaturePartTransactionNrs.get(hash);
		auto it = mBodyBytesTransactionNrs.find(transaction.getBodyBytes());
		//if (pair.has_value()) 
		if (it != mBodyBytesTransactionNrs.end())
		{
			// hash collision check
			auto storedTransaction = FileBasedProvider::getInstance()->findBlockchain(mCommunityId)->getTransactionForId(it->second);
			if (storedTransaction->getConfirmedTransaction()->getGradidoTransaction()->isTheSame(transaction)) {
				return true;
			} else {
				LOG_F(WARNING, "Hash collision detected, %s and %s have the same hash",
					toJsonString(transaction, true).c_str(),
					toJsonString(*storedTransaction->getConfirmedTransaction(), true).c_str()
				);
			}
		}
		return false;
	}

	SignatureOctet TransactionHash::deriveHash(const gradido::data::GradidoTransaction& transaction) const
	{
		if (transaction.getSignatureMap().getSignaturePairs().size()) {
			return transaction.getSignatureMap().getSignaturePairs().front().getSignature()->calculateHash();
		} else {
			return transaction.getBodyBytes()->calculateHash();
		}
	}
}