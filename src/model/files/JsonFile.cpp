#include "JsonFile.h"
#include "../../SystemExceptions.h"

#include <rapidjson/istreamwrapper.h>

#include <fstream>

using namespace rapidjson;

namespace model {
	namespace files {
		JsonFile::JsonFile(const std::string& fileName)
			: mFileName(fileName), mJson(kObjectType)
		{

		}
		Document& JsonFile::load()
		{
			std::ifstream fileStream(mFileName);
			if (!fileStream.is_open()) {
				throw FileNotFoundException("cannot open json file", mFileName.data());
			}
			IStreamWrapper isw(fileStream);
			mJson.Clear();
			mJson.ParseStream(isw);
			if (mJson.HasParseError()) {
				throw RapidjsonParseErrorException("error parsing json file", mJson.GetParseError(), mJson.GetErrorOffset())
					.setRawText(std::string((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>()));
			}
		}

		/*void JsonFile::save()
		{
			throw std::runtime_error("not implemented yet");
		}*/
	}
}