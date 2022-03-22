#ifndef __GRADIDO_NODE_MODEL_APOLLO_DECAY_H
#define __GRADIDO_NODE_MODEL_APOLLO_DECAY_H

#include "gradido_blockchain/lib/Decay.h"
#include "Poco/Timestamp.h"
#include "rapidjson/document.h"

namespace model {
	namespace Apollo {
		extern const char* jsDateTimeFormat;

		class Decay
		{
		public:
			Decay(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance);
			~Decay();

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			inline mpfr_ptr getDecayAmount() const { return mDecayAmount; }
			

		protected:
			Poco::Timestamp mDecayStart;
			Poco::Timestamp mDecayEnd;
			mpfr_ptr mDecayAmount;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_DECAY_H