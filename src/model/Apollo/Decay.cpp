#include "Decay.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include <chrono>

using namespace rapidjson;
using namespace std::chrono;

namespace model {
	namespace Apollo {
		const char* jsDateTimeFormat = "%FT%T";
		static const Timepoint DECAY_START_TIME = DataTypeConverter::dateTimeStringToTimePoint("2021-05-13 17:46:31");

		std::string formatJsCompatible(Timepoint date)
		{
			auto dateString = DataTypeConverter::timePointToString(date, jsDateTimeFormat);
			return dateString;
			// return dateString.substr(0, dateString.find_last_of('.'));
		}

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
			decay.AddMember("start", Value(formatJsCompatible(mDecayStart).data(), alloc), alloc);
			decay.AddMember("end", Value(formatJsCompatible(mDecayEnd).data(), alloc), alloc);
			decay.AddMember("duration", duration_cast<seconds>(mDecayEnd - mDecayStart).count(), alloc);
			decay.AddMember("__typename", "Decay", alloc);
			return std::move(decay);
		}
	}
}