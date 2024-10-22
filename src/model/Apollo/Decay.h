#ifndef __GRADIDO_NODE_MODEL_APOLLO_DECAY_H
#define __GRADIDO_NODE_MODEL_APOLLO_DECAY_H

#include "gradido_blockchain/GradidoUnit.h"
#include "rapidjson/document.h"

namespace model {
	namespace Apollo {
		extern const char* jsDateTimeFormat;

		class Decay
		{
		public:
			Decay(Decay* parent);
			Decay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance);
			~Decay();

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			inline GradidoUnit getDecayAmount() const { return mDecayAmount; }
			

		protected:
			Timepoint mDecayStart;
			Timepoint mDecayEnd;
			GradidoUnit mDecayAmount;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_DECAY_H