#ifndef DEBUGGERDATA_H
#define DEBUGGERDATA_H

#include <list>
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

struct Breakpoint {
	enum Type { BREAKPOINT, WATCHPOINT_MEMREAD, WATCHPOINT_MEMWRITE,
	            WATCHPOINT_IOREAD, WATCHPOINT_IOWRITE, CONDITION };
	Type type;
	QString id;
	quint16 address;
	// end for watchpoint region
	quint16 regionEnd;
	// gui specific condition variables
	qint8 ps;
	qint8 ss;
	qint16 segment;
	// general condition
	QString condition;
	// compare content
	bool operator==(const Breakpoint &bp) const;
};

class Breakpoints
{
public:
	void clear();

	void setMemoryLayout(MemoryLayout* ml);
	void setBreakpoints(const QString& str);
	QString mergeBreakpoints(const QString& str);
	int breakpointCount();
	bool isBreakpoint(quint16 addr, QString *id = nullptr, bool checkSlot = true);
	bool isWatchpoint(quint16 addr, QString *id = nullptr, bool checkSlot = true);

	/* xml session file functions */
	void saveBreakpoints(QXmlStreamWriter& xml);
	void loadBreakpoints(QXmlStreamReader& xml);

	int findBreakpoint(quint16 addr);

	static QString createSetCommand(Breakpoint::Type type, quint16 address,
	                                qint8 ps = -1, qint8 ss = -1, qint16 segment = -1,
	                                int endRange = -1, QString condition = QString());
	static QString createRemoveCommand(const QString& id);

	const Breakpoint& getBreakpoint(int index);
	bool inCurrentSlot(const Breakpoint& bp);
private:

	std::vector<Breakpoint> breakpoints;
	MemoryLayout* memLayout = nullptr;

	void parseCondition(Breakpoint& bp);
	void insertBreakpoint(Breakpoint& bp);
};

#endif // DEBUGGERDATA_H
