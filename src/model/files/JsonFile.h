#ifndef __GRADIDO_NODE_MODEL_FILES_JSON_FILE_H
#define __GRADIDO_NODE_MODEL_FILES_JSON_FILE_H

#include "rapidjson/document.h"
#include <string>

namespace model {
	namespace files {
		/*!
		 * \author einhornimmond
		 *
		 * \date 2024-09-17
		 *
		 * \brief load configuration from file in json format, can also store it back, after adding or update elements
		 */
		class JsonFile
		{
		public:
			//! \param fileName with full path 
			JsonFile(const std::string& fileName);
			//! load from json file
			rapidjson::Document& load();

			rapidjson::Document& getJson() { return mJson; }
			const rapidjson::Document& getJson() const { return mJson; }

			//! save to json
			//void save();

		protected:
			std::string mFileName;
			rapidjson::Document mJson;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_JSON_FILE_H