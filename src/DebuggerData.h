#ifndef DEBUGGERDATA_H
#define DEBUGGERDATA_H

#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <optional>
#include <vector>

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

struct AddressRange {
	explicit AddressRange(uint16_t addr) : start(addr) {}
	AddressRange(uint16_t b, std::optional<uint16_t> e) : start(b), end(e) {}

	uint16_t start;                // a single address when end is not used
	std::optional<uint16_t> end;   // end point is inclusive

	[[nodiscard]] bool contains(uint16_t addr) const {
		if (!end) return start == addr;
		return start <= addr && addr <= *end;
	}

	constexpr bool operator==(const AddressRange &other) const
	{
		if (start != other.start) return false;
		if (end != other.end) return false;
		return true;
	}
};

struct Slot {
    std::optional<int8_t> ps; // primary slot, always 0..3 or empty if it is a mask
    std::optional<int8_t> ss; // secundary slot, 0..3 or empty when the primary slot is not expanded
};

struct Breakpoint {
	enum Type { BREAKPOINT, WATCHPOINT_MEMREAD, WATCHPOINT_MEMWRITE,
	            WATCHPOINT_IOREAD, WATCHPOINT_IOWRITE, CONDITION };
	Type type;
	QString id;
	std::optional<AddressRange> range;
	// GUI specific condition variables
	Slot slot;
	std::optional<uint8_t> segment; 
	// general condition
	QString condition;
	// compare content
	bool operator==(const Breakpoint &bp) const;

    void setAddress(uint16_t start, std::optional<uint16_t> rangeEnd = {}) {
        range = AddressRange(start, rangeEnd);
    }
};

class Breakpoints
{
public:
	void clear();

	void setMemoryLayout(MemoryLayout* ml);
	void setBreakpoints(const QString& str);
	QString mergeBreakpoints(const QString& str);
	int breakpointCount();
	bool isBreakpoint(uint16_t addr, QString *id = nullptr, bool checkSlot = true);
	bool isWatchpoint(uint16_t addr, QString *id = nullptr, bool checkSlot = true);

	/* xml session file functions */
	void saveBreakpoints(QXmlStreamWriter& xml);
	void loadBreakpoints(QXmlStreamReader& xml);

	std::optional<uint16_t> findBreakpoint(uint16_t addr);

	static QString createSetCommand(Breakpoint::Type type, std::optional<AddressRange> range = {},
	                                Slot slot = {}, std::optional<uint8_t> segment = {},
                                    QString condition = {});
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
