// $Id$

#include "DebuggerData.h"


void Breakpoints::setBreakpoints(QByteArray& str)
{
	QList<QByteArray> bps = str.split('\n');
	
	breakpoints.clear();
	for ( QList<QByteArray>::Iterator it = bps.begin(); it != bps.end(); ++it ) {
		QString bpName;
		int p, q;

		p = (*it).indexOf(' ');
		bpName = (*it).left(p).trimmed();
		if(bpName.isEmpty()) break;
		q = (*it).indexOf(' ', p+1);
		if(q==-1) {
			breakpoints[bpName].address = (*it).mid(p).toUShort(0,0);
		} else {
			breakpoints[bpName].address = (*it).mid(p, q).trimmed().toUShort(0,0);
			breakpoints[bpName].condition = (*it).mid(q).trimmed();
		}
	}
}

int Breakpoints::breakpointCount()
{
	return breakpoints.size();
}

bool Breakpoints::isBreakpoint(quint16 addr)
{
	for(BreakpointMap::const_iterator it = breakpoints.constBegin(); it != breakpoints.constEnd(); ++it) {
		if(it.value().address == addr)
			return TRUE;
	}
	
	return FALSE;
}
