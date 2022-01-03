#include "TransactionCreation.h"
#include "GradidoBlock.h"
#include "../../controller/Group.h"

#include "../../lib/BinTextConverter.h"

#include "../../SingletonManager/GroupManager.h"

namespace model {
	TransactionCreation::TransactionCreation(
		const proto::gradido::GradidoCreation& transaction,
		const proto::gradido::SignatureMap& sigMap)
		: mProtoCreation(transaction), mSignatureMap(sigMap)
	{

	}

	bool TransactionCreation::validate(TransactionValidationLevel level)
	{
		auto gm = GroupManager::getInstance();

		// single
		auto recipiantPubkey = mProtoCreation.recipiant().pubkey();
		std::string zeroPubkey(32, 0);
		if (recipiantPubkey == zeroPubkey) {
			addError(new Error(__FUNCTION__, "recipiant pubkey is zero"));
			return false;
		}
		auto sigPairs = mSignatureMap.sigpair();
		for (auto it = sigPairs.begin(); it != sigPairs.end(); it++) {
			if (recipiantPubkey == it->pubkey()) {
				addError(new Error(__FUNCTION__, "recipiant don't allow to sign creation transaction"));
				return false;
			}
		}
		
		if (mProtoCreation.recipiant().amount() > 10000000) {
			addError(new Error(__FUNCTION__, "creation more than 1.000 GDD per creation not allowed"));
			return false;
		}
		
		if ((level & TRANSACTION_VALIDATION_DATE_RANGE) == TRANSACTION_VALIDATION_DATE_RANGE) {
			
			Poco::DateTime targetDate = Poco::Timestamp(mProtoCreation.target_date().seconds() * Poco::Timestamp::resolution());

			if (mGradidoBlock) {
				Poco::DateTime received = mGradidoBlock->getReceived();

				if (targetDate.year() == received.year())
				{
					if (targetDate.month() + 2 < received.month()) {
						addError(new Error(__FUNCTION__, "year is the same, target date month is more than 2 month in past"));
						return false;
					}
					if (targetDate.month() > received.month()) {
						addError(new Error(__FUNCTION__, "year is the same, target date month is in future"));
						return false;
					}
				}
				else if (targetDate.year() > received.year())
				{
					addError(new Error(__FUNCTION__, "target date year is in future"));
					return false;
				}
				else if (targetDate.year() + 1 < received.year())
				{
					addError(new Error(__FUNCTION__, "target date year is in past"));
					return false;
				}
				else
				{
					// target_date.year +1 == now.year
					if (targetDate.month() + 2 < received.month() + 12) {
						addError(new Error(__FUNCTION__, "target date is more than 2 month in past"));
						return false;
					}
				}

				auto pubkey = mProtoCreation.recipiant().pubkey();
				auto groups = gm->findAllGroupsWhichHaveTransactionsForPubkey(pubkey);
				uint64_t sum = mProtoCreation.recipiant().amount();
				for (auto it = groups.begin(); it != groups.end(); it++) {
					try {
						sum += (*it)->calculateCreationSum(pubkey, targetDate.month(), targetDate.year(), received);
					}
					catch (std::exception& ex) {
						printf("exception: %s\n", ex.what());
					}
				}
				auto id = mGradidoBlock->getID();
				int lastId = 0;
				if (!mGroupRoot.isNull() && mGroupRoot->getLastTransaction()) {
					lastId = mGroupRoot->getLastTransaction()->getID();
				}
				if (mGradidoBlock->getID() <= lastId) {
					// this transaction was already added to blockchain and therefor also added in calculateCreationSum
					sum -= mProtoCreation.recipiant().amount();
				}
				// TODO: replace with variable, state transaction for group
				if (sum > 10000000) {
					addError(new Error(__FUNCTION__, "creation more than 1.000 GDD per month not allowed"));
					return false;
				}
			}
		}

		return true;
	}

	std::vector<uint32_t> TransactionCreation::getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer)
	{
		std::vector<uint32_t> addressIndices;
		auto index = addressIndexContainer->getOrAddIndexForAddress(getRecipiantPubkey());
		if (!index) {
			std::string hexPubkey = convertBinToHex(getRecipiantPubkey());
			addError(new ParamError(__FUNCTION__, "cannot find address index for", hexPubkey.data()));
		}
		else {
			addressIndices.push_back(index);
		}
		return addressIndices;
	}
}