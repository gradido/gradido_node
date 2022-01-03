#include "Exceptions.h"

POCO_IMPLEMENT_EXCEPTION(TransactionLoadingException, Poco::Exception,
	"Error loading transaction")