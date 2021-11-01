#include "TransactionTransfer.h"
#include "GradidoBlock.h"
#include <unordered_map>

#include "../../controller/AddressIndex.h"
#include "../../lib/BinTextConverter.h"
#include "../../lib/DataTypeConverter.h"
#include "../../SingletonManager/OrderingManager.h"
#include "../../SingletonManager/GroupManager.h"
#include <stdexcept>

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

		if ((level & TRANSACTION_VALIDATION_SINGLE_PREVIOUS) == TRANSACTION_VALIDATION_SINGLE_PREVIOUS) {
			// TODO: check if enough gradidos exist on account of user
		}

		if ((level & TRANSACTION_VALIDATION_PAIRED) == TRANSACTION_VALIDATION_PAIRED) {
			auto om = OrderingManager::getInstance();
			auto gm = GroupManager::getInstance();
			auto pairedTransactionId = getPairedTransactionId();
			if (pairedTransactionId == 0) {
				addError(new Error(__FUNCTION__, "paired transaction id is zero"));
				return false;
			}

			// we must wait until this node receive also the pair transaction
			Poco::Timestamp timeout = Poco::Timestamp() + Poco::Timespan(MAGIC_NUMBER_TRANSFER_CROSS_GROUP_WAIT_ON_PAIR_SECONDS, 0);
			Poco::AutoPtr<GradidoTransaction> pairTransaction;
			while (Poco::Timestamp() < timeout) {
				pairTransaction = om->findPairedTransaction(pairedTransactionId);
				Poco::Thread::sleep(MAGIC_NUMBER_TRANSFER_CROSS_GROUP_WAIT_ON_PAIR_SLEEPTIME_MILLISECONDS);
			}
			// after timeout we don't get the paired transaction so it was maybe a hoax (the other half doesn't exist), 
			// we decide the transaction is invalid
			// TODO: give it a second chance if iota should be once really badly delay maybe it is possible to find it out with iota node info
			// include "iota/HTTPApi.h"
			// iota::getNodeInfo 
			if (pairTransaction.isNull()) {
				addError(new Error(__FUNCTION__, "cannot find pairing transaction"));
				return false;
			}
			auto otherGroup = gm->findGroup(getOtherGroup());
			TransactionValidationLevel pairValidationLevel = TRANSACTION_VALIDATION_SINGLE;
			if (!otherGroup.isNull()) {
				pairValidationLevel = TRANSACTION_VALIDATION_SINGLE_PREVIOUS;
			}
			if (!pairTransaction->validate(pairValidationLevel)) {
				addError(new Error(__FUNCTION__, "pair transaction isn't valid"));
				return false;
			}
			if (!pairTransaction->getTransactionBody()->getTransfer()->isBelongTo(this)) {
				addError(new Error(__FUNCTION__, "pair transaction don't belong to us"));
				return false;
			}
			// so we know for sure the paired transaction exist and is valid
			// TODO: if the other transaction belongs to a group on which we don't listen, we need to make contact to another Gradido Node
			// which is listening to this transaction to check if sender has enough Gradidos
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

	const proto::gradido::LocalTransfer TransactionTransfer::getTransfer() const
	{
		if (mProtoTransfer.has_local()) {
			return mProtoTransfer.local();
		}
		else if (isCrossGroupTransfer()) {
			return getCrossGroupTransfer().transfer();
		}
		throw std::runtime_error("invalid transfer transaction");
	}
	const proto::gradido::CrossGroupTransfer TransactionTransfer::getCrossGroupTransfer() const
	{
		if (mProtoTransfer.has_inbound()) {
			return mProtoTransfer.inbound();
		}
		else if (mProtoTransfer.has_outbound()) {
			return mProtoTransfer.outbound();
		}
		throw std::runtime_error("invalid cross group transfer transaction");
	}

	std::string TransactionTransfer::getOtherGroup() const
	{
		if (isCrossGroupTransfer()) {
			return getCrossGroupTransfer().other_group();
		}
		else {
			return "";
		}
	}

	Poco::Timestamp TransactionTransfer::getPairedTransactionId() const
	{
		if (isCrossGroupTransfer()) {
			return DataTypeConverter::convertFromProtoTimestamp(getCrossGroupTransfer().paired_transaction_id());
		}
		else {
			return 0;
		}
	}

	bool TransactionTransfer::isBelongTo(const TransactionTransfer* paired) const
	{
		auto transfer = getTransfer();
		auto otherTransfer = paired->getTransfer();
		if (transfer.sender().amount() != otherTransfer.sender().amount()) {
			return false;
		}
		if (getOtherGroup() == paired->getOtherGroup()) {
			return false;
		}
		if (getPairedTransactionId() != paired->getPairedTransactionId()) {
			return false;
		}
		if (
			mProtoTransfer.has_local() || 
			paired->mProtoTransfer.has_local() || 
			// check if booth has the same type (booth inbound or booth outbound) 
			mProtoTransfer.has_inbound() != paired->mProtoTransfer.has_outbound()
			) {
			return false;
		}
		if (transfer.sender().pubkey() != otherTransfer.sender().pubkey()) {
			return false;
		}
		if (transfer.recipiant() != otherTransfer.recipiant()) {
			return false;
		}
		return true;
	}

}
