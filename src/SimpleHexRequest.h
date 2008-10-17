#ifndef SIMPLEHEXREQUEST_H
#define SIMPLEHEXREQUEST_H

#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"

/**  
 * A set of helper classes if code needs to read openmsx debugables into an array of unsigned chars
 * if class A needs to read a given debugable then this schem will suffice
 * - Class A inherits from SimpleHexRequestUser
 * - Class A can reimplement the DataHexRequestReceived if it wants to react when new data has arrived
 * - Class A can reimplement the DataHexRequestCanceled if it wants to react to failures of the request
 * - to read the debuggable into the memmory just create a new SimpleHexRequest, fi.
 *	new SimpleHexRequest("{VDP status regs}",0,16,statusregs, *this);
 *
 */

class SimpleHexRequestUser
{
protected:
	virtual void DataHexRequestReceived();
	virtual void DataHexRequestCanceled();
	friend class SimpleHexRequest;
};

class SimpleHexRequest : public ReadDebugBlockCommand
{
public:
	SimpleHexRequest(const QString& debuggable,  unsigned size,
	           unsigned char* target, SimpleHexRequestUser& user_);
	SimpleHexRequest(const QString& debuggable, unsigned offset_, unsigned size,
	           unsigned char* target, SimpleHexRequestUser& user_);

	virtual void replyOk(const QString& message);
	virtual void cancel();

	unsigned offset;

private:
	SimpleHexRequestUser& user;
};

#endif /* SIMPLEHEXREQUEST_H */

