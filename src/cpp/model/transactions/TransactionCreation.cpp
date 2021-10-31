#include "TransactionCreation.h"
#include "GradidoBlock.h"
#include "../../controller/Group.h"

#include "../../lib/BinTextConverter.h"

namespace model {
	TransactionCreation::TransactionCreation(
		const proto::gradido::GradidoCreation& transaction,
		const proto::gradido::SignatureMap& sigMap)
		: mProtoCreation(transaction), mSignatureMap(sigMap)
	{

	}

	bool TransactionCreation::validate(TransactionValidationLevel level)
	{
		// single
		auto receiverPubkey = mProtoCreation.recipiant().pubkey();
		auto sigPairs = mSignatureMap.sigpair();
		for (auto it = sigPairs.begin(); it != sigPairs.end(); it++) {
			if (receiverPubkey == it->pubkey()) {
				addError(new Error(__FUNCTION__, "receiver don't allow to sign creation transaction"));
				return false;
			}
		}
		
		if (mProtoCreation.recipiant().amount() > 10000000) {
			addError(new Error(__FUNCTION__, "creation more than 1.000 GDD per creation not allowed"));
			return false;
		}
		
		if ((level & TRANSACTION_VALIDATION_DATE_RANGE) == TRANSACTION_VALIDATION_DATE_RANGE) {
			
			Poco::DateTime targetDate = Poco::Timestamp(mProtoCreation.target_date().seconds());
				
			auto pubkey = mProtoCreation.recipiant().pubkey();
			auto groups = getGroups();
			uint64_t sum = mProtoCreation.recipiant().amount();
			for (auto it = groups.begin(); it != groups.end(); it++) {
				sum += (*it)->calculateCreationSum(pubkey, targetDate.month(), targetDate.year());
			}
			// TODO: replace with variable, state transaction for group
			if (sum > 10000000) {
				addError(new Error(__FUNCTION__, "creation more than 1.000 GDD per month not allowed"));
				return false;
			}

			if (mGradidoBlock) {
				Poco::DateTime received = mGradidoBlock->getReceived();
				if (targetDate.year() == received.year())
				{
					if (targetDate.month() + 3 < received.month()) {
						addError(new Error(__FUNCTION__, "year is the same, target date month is more than 3 month in past"));
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
					if (targetDate.month() + 3 < received.month() + 12) {
						addError(new Error(__FUNCTION__, "target date is more than 3 month in past"));
						return false;
					}
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