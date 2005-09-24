// $Id$

#ifndef _DEBUGGERDATA_H
#define _DEBUGGERDATA_H

#include <QLinkedList>
#include <QByteArray>
#include <QString>


class MemoryLayout
{
public:
	MemoryLayout();

	char primarySlot[4];
	char secondarySlot[4];
	unsigned char mapperSegment[4];
	
	bool isSubslotted[4];
	int mapperSize[4][4];
	
};


class Breakpoints
{
public:
	Breakpoints() { memLayout = NULL; };

	void setMemoryLayout(MemoryLayout *ml);
	void setBreakpoints(QByteArray& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr);
	const QString idString(quint16 addr);

	int findBreakpoint(quint16 addr);
	int findNextBreakpoint();

private:

	struct Breakpoint {
		QByteArray id;
		quint16 address;
		char ps;
		char ss;
		qint16 segment;
		QByteArray condition;
	};
	typedef QLinkedList<Breakpoint> BreakpointList;

	BreakpointList breakpoints;
	MemoryLayout *memLayout;
	
	void parseCondition(Breakpoint& bp);
	void insertBreakpoint(Breakpoint& bp);
	QByteArray getNextArgument(QByteArray& data, int& pos);
	bool inCurrentSlot(const Breakpoint& bp);
};


#endif // _DEBUGGERDATA_H
