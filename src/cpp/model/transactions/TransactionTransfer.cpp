#include "TransactionTransfer.h"
#include <unordered_map>

namespace model {
	TransactionTransfer::TransactionTransfer(
		const model::messages::gradido::Transfer& transafer,
		const model::messages::gradido::SignatureMap& sigMap)
		: mProtoTransfer(transafer), mSignatureMap(sigMap)
	{

	}

	bool TransactionTransfer::validate(TransactionValidationLevel level)
	{
		std::unordered_map<std::string, std::string> pubkeys_map;
		int mapInsertCount = 0;

		auto senderAmounts = mProtoTransfer.senderamounts();
		int senderSum = 0;

		for (auto it = senderAmounts.begin(); it != senderAmounts.end(); it++) 
		{
			senderSum += it->amount();
			pubkeys_map.insert(std::pair<std::string, std::string>(it->ed25519_sender_pubkey(), "sender"));
			mapInsertCount++;
		}

		auto receiverAmounts = mProtoTransfer.receiveramounts();
		int receiverSum = 0;
		for (auto it = receiverAmounts.begin(); it != receiverAmounts.end(); it++) 
		{
			receiverSum += it->amount();
			pubkeys_map.insert(std::pair<std::string, std::string>(it->ed25519_receiver_pubkey(), "receiver"));
			mapInsertCount++;
		}

		if (mapInsertCount != pubkeys_map.size()) {
			addError(new Error(__FUNCTION__, "duplicate pubkey in sender and/or receiver"));
			return false;
		}
		if (senderSum != receiverSum) {
			addError(new Error(__FUNCTION__, "sender amounts don't match receiver amounts!"));
			return false;
		}

		auto sigpairs = mSignatureMap.sigpair();
		int pubkey_founds = 0;
		for (auto it = sigpairs.begin(); it != sigpairs.end(); it++) {
			if (pubkeys_map.find(it->ed25519()) != pubkeys_map.end()) {
				pubkey_founds++;
			}
		}
		if (pubkey_founds != pubkeys_map.size()) {
			addError(new Error(__FUNCTION__, "missing signature for sender/receiver"));
			return false;
		}

		if (level == TRANSACTION_VALIDATION_SINGLE_PREVIOUS) {

		}

		return true;
	}
}