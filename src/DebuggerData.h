#ifndef DEBUGGERDATA_H
#define DEBUGGERDATA_H

#include <QLinkedList>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

struct MemoryLayout
{
	MemoryLayout();

	int primarySlot[4];
	int secondarySlot[4];
	int mapperSegment[4];
	int romBlock[8];

	bool isSubslotted[4];
	int mapperSize[4][4];
};

class Breakpoints
{
public:
	Breakpoints();

	enum Type { BREAKPOINT = 0, WATCHPOINT_MEMREAD, WATCHPOINT_MEMWRITE,
	            WATCHPOINT_IOREAD, WATCHPOINT_IOWRITE, CONDITION };

	void clear();

	void setMemoryLayout(MemoryLayout* ml);
	void setBreakpoints(const QString& str);
	QString mergeBreakpoints(const QString& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr, QString *id = nullptr);
	bool isWatchpoint(quint16 addr, QString *id = nullptr);

	/* xml session file functions */
	void saveBreakpoints(QXmlStreamWriter& xml);
	void loadBreakpoints(QXmlStreamReader& xml);

	int findBreakpoint(quint16 addr);
	int findNextBreakpoint();

	static QString createSetCommand(Type type, int address,
	                                char ps = -1, char ss = -1, int segment = -1,
	                                int endRange = -1, QString condition = QString());
	static QString createRemoveCommand(const QString& id);

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
		// compare content
		bool operator==(const Breakpoint &bp) const;
	};
	using BreakpointList = QLinkedList<Breakpoint>;

	BreakpointList breakpoints;
	MemoryLayout* memLayout;

	void parseCondition(Breakpoint& bp);
	void insertBreakpoint(Breakpoint& bp);
	bool inCurrentSlot(const Breakpoint& bp);
};

#endif // DEBUGGERDATA_H
