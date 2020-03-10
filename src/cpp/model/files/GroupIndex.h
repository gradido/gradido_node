#ifndef __GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H


#include "Poco/Util/LayeredConfiguration.h"

namespace model {
	namespace files {
		/*! 
		 * \author Dario Rekowski
		 * 
		 * \date 2020-01-29
		 * 
		 * \brief load group.index file with group public key = folder assignments
		 * 
		 * load ~/.gradido/group.index as Poco Property File Configuration \n
		 * txt format, created by user\n
		 * containing in this format: \n
		 * &lt;group public key&gt; = &lt;folder name&gt;\n
		 * example: \n
		 * <code>FLWHbJcMCKq2LF14K5484zkyMn5LZyFPFbd2n6DE2Zwp = group_gradido_academy</code> \n
		 * \n
		 * TODO: adding function to adding group folder pair and save changed group.index file\n
		 *
		 */
		class GroupIndex
		{
		public:
			//! load group.index into memory as Poco Layered Configuration
			GroupIndex(const std::string& path);

			//! \return Poco AutoPtr to Poco Layered Configuration (simple key = value pair list access via find)
			inline Poco::AutoPtr<Poco::Util::LayeredConfiguration> getConfig() { return mConfig; }

		protected:
			Poco::AutoPtr<Poco::Util::LayeredConfiguration> mConfig;

		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_GROUP_INDEX_H