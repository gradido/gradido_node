#include "Decay.h"
#include "gradido_blockchain/MemoryManager.h"
#include "Poco/Timespan.h"

#include "gradido_blockchain/model/protobufWrapper/TransactionBase.h"

using namespace rapidjson;

namespace model {
	namespace Apollo {
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
			std::string balanceString;
			model::gradido::TransactionBase::amountToString(&balanceString, mDecayAmount);
			decay.AddMember("balance", Value(balanceString.data(), alloc), alloc);
			decay.AddMember("decayStart", mDecayStart.epochTime(), alloc);
			decay.AddMember("decayEnd", mDecayEnd.epochTime(), alloc);
			decay.AddMember("decayDuration", Poco::Timespan(mDecayEnd - mDecayStart).totalSeconds(), alloc);
			decay.AddMember("__typename", "Decay", alloc);
			return std::move(decay);
		}
	}
}