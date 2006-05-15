// $Id$

#ifndef DEBUGGERDATA_H
#define DEBUGGERDATA_H

#include <QLinkedList>
#include <QString>


struct MemoryLayout
{
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
	Breakpoints();

	void setMemoryLayout(MemoryLayout *ml);
	void setBreakpoints(const QString& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr);
	const QString idString(quint16 addr);

	int findBreakpoint(quint16 addr);
	int findNextBreakpoint();

private:
	struct Breakpoint {
		QString id;
		quint16 address;
		char ps;
		char ss;
		qint16 segment;
		QString condition;
	};
	typedef QLinkedList<Breakpoint> BreakpointList;

	BreakpointList breakpoints;
	MemoryLayout* memLayout;
	
	void parseCondition(Breakpoint& bp);
	void insertBreakpoint(Breakpoint& bp);
	QString getNextArgument(QString& data, int& pos);
	bool inCurrentSlot(const Breakpoint& bp);
};

#endif // DEBUGGERDATA_H
