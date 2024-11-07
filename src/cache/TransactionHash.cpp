#include "TransactionHash.h"

#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/NodeTransactionEntry.h"
#include "gradido_blockchain/data/GradidoTransaction.h"

#include "loguru/loguru.hpp"

using namespace gradido;
using namespace blockchain;
using namespace data;

namespace cache {
	TransactionHash::TransactionHash(std::string_view communityId)
		: mSignaturePartTransactionNrs(MAGIC_NUMBER_SIGNATURE_CACHE), mCommunityId(communityId)
	{

	}

	TransactionHash::~TransactionHash()
	{
		mSignaturePartTransactionNrs.clear();
	}

	void TransactionHash::push(const gradido::data::ConfirmedTransaction& confirmedTransaction)
	{
		auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
		auto firstSignature = gradidoTransaction->getSignatureMap().getSignaturePairs().front().getSignature();
		auto pair = mSignaturePartTransactionNrs.get(*firstSignature);
		if (pair.has_value()) {
			throw GradidoAlreadyExist("key already exist");
		}
		mSignaturePartTransactionNrs.add(*firstSignature, confirmedTransaction.getId());
	}

	bool TransactionHash::has(const data::GradidoTransaction& transaction) const
	{
		// get first signature from transaction
		auto firstSignature = transaction.getSignatureMap().getSignaturePairs().front().getSignature();
		auto pair = mSignaturePartTransactionNrs.get(SignatureOctet(*firstSignature));
		if (pair.has_value()) {
			// hash collision check
			auto transaction = FileBasedProvider::getInstance()->findBlockchain(mCommunityId)->getTransactionForId(pair.value());
			auto firstSignatureFromFoundedTransaction = transaction->getConfirmedTransaction()->getGradidoTransaction()->getSignatureMap().getSignaturePairs().front().getSignature();
			if (firstSignatureFromFoundedTransaction->isTheSame(firstSignature)) {
				return true;
			}
			else {
				LOG_F(INFO, "Hash collision detected, %s and %s have the same first 8 Bytes",
					firstSignature->convertToHex().data(), firstSignatureFromFoundedTransaction->convertToHex().data()
				);
			}
		}
		return false;
	}
}