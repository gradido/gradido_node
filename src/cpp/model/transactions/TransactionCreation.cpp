#include "TransactionCreation.h"
#include "Transaction.h"
#include "../../controller/Group.h"

namespace model {
	TransactionCreation::TransactionCreation(
		const model::messages::gradido::TransactionCreation& transaction,
		const model::messages::gradido::SignatureMap& sigMap)
		: mProtoCreation(transaction), mSignatureMap(sigMap)
	{

	}

	bool TransactionCreation::validate(TransactionValidationLevel level)
	{
		// single
		auto receiverPubkey = mProtoCreation.receiveramount().ed25519_receiver_pubkey();
		auto sigPairs = mSignatureMap.sigpair();
		for (auto it = sigPairs.begin(); it != sigPairs.end(); it++) {
			if (receiverPubkey == it->pubkey()) {
				addError(new Error(__FUNCTION__, "receiver don't allow to sign creation transaction"));
				return false;
			}
		}
		// TODO: replace with variable, state transaction for group
		// TODO: start with two month range and 2.000 GDD
		if (mProtoCreation.receiveramount().amount() > 20000000) {
			addError(new Error(__FUNCTION__, "creation more than 2.000 GDD per creation not allowed"));
			return false;
		}
		// date range now 2 Month
		if (level == TRANSACTION_VALIDATION_DATE_RANGE) {
			if (mParent) {
				auto received = mParent->getReceived();
				
				auto pubkey = mProtoCreation.receiveramount().ed25519_receiver_pubkey();
				auto groups = getGroups();
				uint64_t sum = mProtoCreation.receiveramount().amount();
				for (auto it = groups.begin(); it != groups.end(); it++) {
					sum += (*it)->calculateCreationSum(pubkey, received.month(), received.year());
					if (received.month() == 1) {
						sum += (*it)->calculateCreationSum(pubkey, 12, received.year()-1);
					}
					else {
						sum += (*it)->calculateCreationSum(pubkey, received.month() - 1, received.year());
					}
				}
				// TODO: replace with variable, state transaction for group
				if (sum > 20000000) {
					addError(new Error(__FUNCTION__, "creation more than 1.000 GDD per month not allowed"));
					return false;
				}
			}
		}

		//auto identHash = mProtoCreation.ident_hash();
		//int zahl = 0;

		return true;
	}
}