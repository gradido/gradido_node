#include "QueryHeader.h"

namespace hiero {
	QueryHeader::QueryHeader()
		: mResponseType(hiero::ResponseType::ANSWER_ONLY)
	{

	}
	QueryHeader::QueryHeader(ResponseType type)
		: mResponseType(type)
	{

	}
	QueryHeader::QueryHeader(const QueryHeaderMessage& message)
		: mResponseType(message["responseType"_f].value())
	{

	}
	QueryHeader::~QueryHeader()
	{

	}

	QueryHeaderMessage QueryHeader::getMessage() const
	{
		QueryHeaderMessage message;
		message["responseType"_f] = mResponseType;
		return message;
	}
}