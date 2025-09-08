#include "ConsensusMessageChunkInfo.h"
#include "rapidjson.h"
#include "gradido_blockchain/interaction/deserialize/HieroTransactionIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroTransactionIdRole.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"

using namespace gradido;
using namespace interaction;
using namespace rapidjson;
using namespace rapidjson_helper;

namespace hiero {
	static const char* className = "ConsensusMessageChunkInfo";
	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo()
		: mTotal(0), mNumber(0)
	{

	}

	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo(const ConsensusMessageChunkInfoMessage& message)
		: mInitialTransactionID(deserialize::HieroTransactionIdRole(message["initialTransactionID"_f].value())),
		mTotal(message["total"_f].value()), mNumber(message["number"_f].value())
	{
	}

	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo(const TransactionId& initialTransactionId, int32_t total, int32_t number)
		: mInitialTransactionID(initialTransactionId), mTotal(total), mNumber(number)
	{

	}

	ConsensusMessageChunkInfo::ConsensusMessageChunkInfo(const rapidjson::Value& json)
	{
		if (json.HasMember("initial_transaction_id") && json["initial_transaction_id"].IsObject()) {
			const auto& initialTransactionId = json["initial_transaction_id"].GetObject();
			checkMember(initialTransactionId, "account_id", MemberType::STRING, className);
			checkMember(initialTransactionId, "nonce", MemberType::INTEGER, className);
			checkMember(initialTransactionId, "transaction_valid_start", MemberType::STRING, className);
			AccountId accountId(std::string(
				initialTransactionId["account_id"].GetString(), 
				initialTransactionId["account_id"].GetStringLength()
			));
			mInitialTransactionID = TransactionId(
				timestampFromJson(initialTransactionId["transaction_valid_start"]),
				accountId
			);
			mInitialTransactionID.setNonce(initialTransactionId["nonce"].GetInt());

			if (initialTransactionId.HasMember("scheduled") && initialTransactionId["scheduled"].IsBool()) {
				auto scheduled = initialTransactionId["scheduled"].GetBool();
				if (scheduled) {
					mInitialTransactionID.setScheduled();
				}
			}
			
		}
		checkMember(json, "number", MemberType::INTEGER, className);
		checkMember(json, "total", MemberType::INTEGER, className);
		mNumber = json["number"].GetInt();
		mTotal = json["number"].GetInt();
	}

	ConsensusMessageChunkInfo::~ConsensusMessageChunkInfo()
	{

	}

	ConsensusMessageChunkInfoMessage ConsensusMessageChunkInfo::getMessage() const
	{
		ConsensusMessageChunkInfoMessage message;
		message["initialTransactionID"_f] = serialize::HieroTransactionIdRole(mInitialTransactionID).getMessage();
		message["total"_f] = mTotal;
		message["number"_f] = mNumber;
		return message;
	}
}