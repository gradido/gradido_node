#include "TransactionTransfer.h"
#include <unordered_map>

#include "../../controller/AddressIndex.h"
#include "../../lib/BinTextConverter.h"

namespace model {
	TransactionTransfer::TransactionTransfer(
		const proto::gradido::GradidoTransfer& transafer,
		const proto::gradido::SignatureMap& sigMap)
		: mProtoTransfer(transafer), mSignatureMap(sigMap)
	{

	}

	bool TransactionTransfer::validate(TransactionValidationLevel level)
	{
		proto::gradido::LocalTransfer transfer = getTransfer();
		
		if (transfer.sender().amount() <= 0) {
			addError(new Error(__FUNCTION__, "invalid sender amount"));
		}
		if (transfer.sender().pubkey() == transfer.recipiant()) {
			addError(new Error(__FUNCTION__, "sender is recipiant"));
		}

		auto sigpairs = mSignatureMap.sigpair();
		bool sender_pubkey_found = false;
		for (auto it = sigpairs.begin(); it != sigpairs.end(); it++) {
			if (it->pubkey() == transfer.sender().pubkey()) {
				sender_pubkey_found = true;
				break;
			}
		}
		if (!sender_pubkey_found) {
			addError(new Error(__FUNCTION__, "missing signature for sender"));
			return false;
		}

		if (level == TRANSACTION_VALIDATION_SINGLE_PREVIOUS) {
			// TODO: check if enough gradidos exist on account of user
		}

		return true;
	}

	std::vector<uint32_t> TransactionTransfer::getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer)
	{
		std::vector<uint32_t> addressIndices;
		proto::gradido::LocalTransfer transfer = getTransfer();
		
		auto senderIndex = addressIndexContainer->getOrAddIndexForAddress(transfer.sender().pubkey());

		if (!senderIndex) {
			std::string hexPubkey = convertBinToHex(transfer.sender().pubkey());
			addError(new ParamError(__FUNCTION__, "sender, cannot find address index for", hexPubkey.data()));
		} else {
			addressIndices.push_back(senderIndex);
		}
		
		auto recipiantIndex = addressIndexContainer->getOrAddIndexForAddress(transfer.recipiant());
		
		if (!recipiantIndex)
		{
			std::string hexPubkey = convertBinToHex(transfer.recipiant());
			addError(new ParamError(__FUNCTION__, "receiver, cannot find address index for", hexPubkey.data()));
		} 
		else {
			addressIndices.push_back(recipiantIndex);
		}
		return addressIndices;
	}

	const proto::gradido::LocalTransfer TransactionTransfer::getTransfer()
	{
		if (mProtoTransfer.has_local()) {
			return mProtoTransfer.local();
		}
		else if (mProtoTransfer.has_inbound()) {
			return mProtoTransfer.inbound().transfer();
		}
		else if (mProtoTransfer.has_outbound()) {
			return mProtoTransfer.outbound().transfer();
		}
		throw std::exception("invalid transfer transaction");
	}
		
}