#include "Decay.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "magic_enum/magic_enum.hpp"

#include <chrono>

using namespace rapidjson;
using namespace std::chrono;
using namespace magic_enum;

namespace model {
	namespace Apollo {
		const char* jsDateTimeFormat = "%FT%T";
		static const Timepoint DECAY_START_TIME = DataTypeConverter::dateTimeStringToTimePoint("2021-05-13 17:46:31");

		std::string formatJsCompatible(Timepoint date)
		{
			auto dateString = DataTypeConverter::timePointToString(date, jsDateTimeFormat);
			// add Z to say it is UTC
			return dateString + "Z";
			// return dateString.substr(0, dateString.find_last_of('.'));
		}

		Decay::Decay(Decay* parent)
			: mDecayStart(parent->mDecayStart), mDecayEnd(parent->mDecayEnd), mDecayAmount(parent->mDecayAmount), mDecayType(parent->getDecayType())
		{

		}

		Decay::Decay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance, GradidoUnit decayAmount)
			: mDecayStart(decayStart), mDecayEnd(decayEnd), mDecayAmount(decayAmount), mDecayType(decideDecayType(decayStart, decayEnd))
		{
			if (mDecayStart < DECAY_START_TIME) {
				mDecayStart = DECAY_START_TIME;
			}
			if (mDecayEnd < mDecayStart) {
				mDecayEnd = mDecayStart;
			}
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
			decay.AddMember("duration", static_cast<int64_t>(
				duration_cast<seconds>(mDecayEnd - mDecayStart).count()
			), alloc);
			decay.AddMember("type", Value(enum_name(mDecayType).data(), alloc), alloc);
			decay.AddMember("__typename", "Decay", alloc);
			return std::move(decay);
		}

		DecayType Decay::decideDecayType(Timepoint decayStart, Timepoint decayEnd)
		{
			if (decayEnd <= DECAY_START_TIME) {
				return DecayType::BEFORE_START_BLOCK;
			}
			if (decayStart >= DECAY_START_TIME) {
				return DecayType::AFTER_START_BLOCK;
			}
			return DecayType::START_BLOCK_INSIDE;
		}
	}
}