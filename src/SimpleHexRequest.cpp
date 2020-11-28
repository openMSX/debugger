#include "SimpleHexRequest.h"
#include "CommClient.h"


// class SimpleHexRequest

SimpleHexRequest::SimpleHexRequest(
		const QString& debuggable,  unsigned size,
		unsigned char* target, SimpleHexRequestUser& user_)
	: ReadDebugBlockCommand(debuggable, size, target)
	, offset(0)
	, user(user_)
{
	CommClient::instance().sendCommand(this);
}

SimpleHexRequest::SimpleHexRequest(
		const QString& debuggable, unsigned offset_, unsigned size,
		unsigned char* target, SimpleHexRequestUser& user_)
	: ReadDebugBlockCommand(debuggable, offset_, size, target)
	, offset(offset_)
	, user(user_)
{
	CommClient::instance().sendCommand(this);
}

void SimpleHexRequest::replyOk(const QString& message)
{
	copyData(message);
	user.DataHexRequestReceived();
	delete this;
}

void SimpleHexRequest::cancel()
{
	user.DataHexRequestCanceled();
	delete this;
}


// class SimpleHexRequestUser

void SimpleHexRequestUser::DataHexRequestReceived()
{
}

void SimpleHexRequestUser::DataHexRequestCanceled()
{
}
