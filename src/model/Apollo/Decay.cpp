#include "Decay.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include <chrono>

using namespace rapidjson;
using namespace std::chrono;

namespace model {
	namespace Apollo {
		const char* jsDateTimeFormat = "%Y-%m-%dT%H:%M:%S.%i%z";
		static const Timepoint DECAY_START_TIME = DataTypeConverter::dateTimeStringToTimePoint("2021-05-13 17:46:31");

		Decay::Decay(Decay* parent)
			: mDecayStart(parent->mDecayStart), mDecayEnd(parent->mDecayEnd), mDecayAmount(parent->mDecayAmount)
		{

		}
		Decay::Decay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance)
			: mDecayStart(decayStart), mDecayEnd(decayEnd)
		{
			mDecayAmount = startBalance.calculateDecay(mDecayStart, mDecayEnd) - startBalance;
		}

		Decay::~Decay()
		{

		}

		Value Decay::toJson(Document::AllocatorType& alloc)
		{
			Value decay(kObjectType);
			decay.AddMember("decay", Value(mDecayAmount.toString().data(), alloc), alloc);
			auto decayStartString = DataTypeConverter::timePointToString(mDecayStart, jsDateTimeFormat);
			decay.AddMember("start", Value(decayStartString.data(), alloc), alloc);
			auto decayEndString = DataTypeConverter::timePointToString(mDecayEnd, jsDateTimeFormat);
			decay.AddMember("end", Value(decayEndString.data(), alloc), alloc);
			decay.AddMember("duration", duration_cast<seconds>(mDecayEnd - mDecayStart).count(), alloc);
			decay.AddMember("__typename", "Decay", alloc);
			return std::move(decay);
		}
	}
}