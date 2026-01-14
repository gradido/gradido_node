#ifndef __GRADIDO_NODE_MODEL_APOLLO_DECAY_H
#define __GRADIDO_NODE_MODEL_APOLLO_DECAY_H

#include "gradido_blockchain/GradidoUnit.h"
#include "rapidjson/document.h"

namespace model {
	namespace Apollo {
		extern const char* jsDateTimeFormat;

		std::string formatJsCompatible(Timepoint date);

		enum DecayType 
		{
			BEFORE_START_BLOCK,
			START_BLOCK_INSIDE,
			AFTER_START_BLOCK
		};

		class Decay
		{
		public:
			Decay(Decay* parent);
			Decay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance, GradidoUnit decayAmount);
			~Decay();

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			inline GradidoUnit getDecayAmount() const { return mDecayAmount; }
			inline DecayType getDecayType() const { return mDecayType; }

		protected:
			static DecayType decideDecayType(Timepoint decayStart, Timepoint decayEnd);

			Timepoint mDecayStart;
			Timepoint mDecayEnd;
			GradidoUnit mDecayAmount;
			DecayType mDecayType;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_DECAY_H