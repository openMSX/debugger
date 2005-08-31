// $Id$

#ifndef _DEBUGGERDATA_H
#define _DEBUGGERDATA_H

#include <QMap>
#include <QByteArray>
#include <QString>

class Breakpoints
{
public:
	Breakpoints() {};
		
	void setBreakpoints(QByteArray& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr);
	
private:

	typedef struct {
		quint16 address;
		QByteArray condition;
	} Breakpoint;
	typedef QMap<QString, Breakpoint> BreakpointMap;

	BreakpointMap breakpoints;
};


#endif // _DEBUGGERDATA_H
