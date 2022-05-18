#ifndef SIMPLEHEXREQUEST_H
#define SIMPLEHEXREQUEST_H

#include "OpenMSXConnection.h"

/**
 * A set of helper classes if code needs to read openMSX debugables into an array of uint8_t's
 * if class A needs to read a given debugable then this schema will suffice
 * - Class A inherits from SimpleHexRequestUser
 * - Class A can reimplement the DataHexRequestReceived if it wants to react when new data has arrived
 * - Class A can reimplement the DataHexRequestCanceled if it wants to react to failures of the request
 * - to read the debuggable into the memory just create a new SimpleHexRequest, fi.
 *	new SimpleHexRequest("{VDP status regs}", 0, 16, statusregs, *this);
 *
 */
class SimpleHexRequestUser
{
protected:
	virtual ~SimpleHexRequestUser() = default;
	virtual void DataHexRequestReceived();
	virtual void DataHexRequestCanceled();
	friend class SimpleHexRequest;
};

class SimpleHexRequest : public ReadDebugBlockCommand
{
public:
	SimpleHexRequest(const QString& debuggable, unsigned size,
	           uint8_t* target, SimpleHexRequestUser& user);
	SimpleHexRequest(const QString& debuggable, unsigned offset, unsigned size,
	           uint8_t* target, SimpleHexRequestUser& user);

	void replyOk(const QString& message) override;
	void cancel() override;

	unsigned offset;

private:
	SimpleHexRequestUser& user;
};

#endif // SIMPLEHEXREQUEST_H
