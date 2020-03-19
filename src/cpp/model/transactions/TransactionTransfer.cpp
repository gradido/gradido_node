#include "TransactionTransfer.h"
#include <unordered_map>

#include "../../controller/AddressIndex.h"
#include "../../lib/BinTextConverter.h"

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

	std::vector<uint32_t> TransactionTransfer::getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer)
	{
		std::vector<uint32_t> addressIndices;
		auto senderAmounts = mProtoTransfer.senderamounts();

		for (auto it = senderAmounts.begin(); it != senderAmounts.end(); it++)
		{
			auto index = addressIndexContainer->getIndexForAddress((*it).ed25519_sender_pubkey());
			if (!index) 
			{
				std::string hexPubkey = convertBinToHex((*it).ed25519_sender_pubkey());
				addError(new ParamError(__FUNCTION__, "sender, cannot find address index for", hexPubkey.data()));
			} 
			else 
			{
				addressIndices.push_back(index);
			}
		}

		auto receiverAmounts = mProtoTransfer.receiveramounts();
		
		for (auto it = receiverAmounts.begin(); it != receiverAmounts.end(); it++)
		{
			auto index = addressIndexContainer->getIndexForAddress((*it).ed25519_receiver_pubkey());
			if (!index) 
			{
				std::string hexPubkey = convertBinToHex((*it).ed25519_receiver_pubkey());
				addError(new ParamError(__FUNCTION__, "receiver, cannot find address index for", hexPubkey.data()));
			} 
			else
			{
				bool exist = false;
				for (auto it = addressIndices.begin(); it != addressIndices.end(); it++) {
					if (index == *it) {
						exist = true;
						break;
					}
				}
				if (!exist) {
					addressIndices.push_back(index);
				}
			}
		}
		return addressIndices;
	}
}