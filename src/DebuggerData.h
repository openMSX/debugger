// $Id$

#ifndef DEBUGGERDATA_H
#define DEBUGGERDATA_H

#include <QLinkedList>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

struct MemoryLayout
{
	MemoryLayout();

	char primarySlot[4];
	char secondarySlot[4];
	unsigned char mapperSegment[4];
	int romBlock[8];

	bool isSubslotted[4];
	int mapperSize[4][4];
};


class Breakpoints
{
public:
	Breakpoints();

	enum Type { BREAKPOINT, WATCHPOINT_IOREAD, WATCHPOINT_IOWRITE,
	            WATCHPOINT_MEMREAD, WATCHPOINT_MEMWRITE };

	void clear();

	void setMemoryLayout(MemoryLayout* ml);
	void setBreakpoints(const QString& str);
	QString mergeBreakpoints(const QString& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr);
	const QString idString(quint16 addr);

	/* xml session file functions */
	void saveBreakpoints(QXmlStreamWriter& xml);
	void loadBreakpoints(QXmlStreamReader& xml);

	int findBreakpoint(quint16 addr);
	int findNextBreakpoint();

private:
	struct Breakpoint {
		Type type;
		QString id;
		quint16 address;
		// end for watchpoint region
		quint16 regionEnd;
		// gui specific condition variables
		char ps;
		char ss;
		qint16 segment;
		// general condition
		QString condition;
	};
	typedef QLinkedList<Breakpoint> BreakpointList;

	BreakpointList breakpoints;
	MemoryLayout* memLayout;

	void parseCondition(Breakpoint& bp);
	void insertBreakpoint(Breakpoint& bp);
	bool inCurrentSlot(const Breakpoint& bp);
};

#endif // DEBUGGERDATA_H
