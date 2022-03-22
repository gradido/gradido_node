#include "Decay.h"
#include "gradido_blockchain/MemoryManager.h"
#include "Poco/Timespan.h"
#include "Poco/DateTimeFormatter.h"

#include "gradido_blockchain/model/protobufWrapper/TransactionBase.h"

using namespace rapidjson;

namespace model {
	namespace Apollo {
		const char* jsDateTimeFormat = "%Y-%m-%dT%H:%M:%S.%i%z";

		Decay::Decay(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance)
			: mDecayStart(decayStart), mDecayEnd(decayEnd), mDecayAmount(nullptr)
		{
			assert(mDecayEnd > mDecayStart);
			auto duration = Poco::Timespan(mDecayEnd - mDecayStart);
			auto mm = MemoryManager::getInstance();
			auto decayFactor = MathMemory::create();
			auto balance = MathMemory::create();
			calculateDecayFactorForDuration(decayFactor->getData(), gDecayFactorGregorianCalender, duration.totalSeconds());
			mpfr_set(balance->getData(), startBalance, gDefaultRound);
			calculateDecayFast(decayFactor->getData(), balance->getData());
			mDecayAmount = mm->getMathMemory();
			mpfr_sub(mDecayAmount, startBalance, balance->getData(), gDefaultRound);
			mpfr_neg(mDecayAmount, mDecayAmount, gDefaultRound);
		}

		Decay::~Decay()
		{
			if (mDecayAmount) {
				MemoryManager::getInstance()->releaseMathMemory(mDecayAmount);
				mDecayAmount = nullptr;
			}
		}

		Value Decay::toJson(Document::AllocatorType& alloc)
		{
			Value decay(kObjectType);
			std::string decayString;
			model::gradido::TransactionBase::amountToString(&decayString, mDecayAmount);
			decay.AddMember("decay", Value(decayString.data(), alloc), alloc);
			auto decayStartString = Poco::DateTimeFormatter::format(mDecayStart, jsDateTimeFormat);
			decay.AddMember("start", Value(decayStartString.data(), alloc), alloc);
			auto decayEndString = Poco::DateTimeFormatter::format(mDecayEnd, jsDateTimeFormat);
			decay.AddMember("end", Value(decayEndString.data(), alloc), alloc);
			decay.AddMember("duration", Poco::Timespan(mDecayEnd - mDecayStart).totalSeconds(), alloc);
			decay.AddMember("__typename", "Decay", alloc);
			return std::move(decay);
		}
	}
}