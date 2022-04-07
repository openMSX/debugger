#include "DebuggerData.h"
#include "Convert.h"
#include "ranges.h"
#include <qglobal.h>
#include <QStringList>
#include <QDebug>
#include <algorithm>
// class MemoryLayout

static const char* const TypeNames[] = { "breakpoint", "memread", "memwrite", "ioread", "iowrite",
                                         "condition" };

MemoryLayout::MemoryLayout()
{
	for (int p = 0; p < 4; ++p) {
		primarySlot[p] = 0;
		secondarySlot[p] = -1;
		mapperSegment[p] = 0;
		isSubslotted[p] = false;
		for (int q = 0; q < 4; ++q) {
			mapperSize[p][q] = 0;
		}
	}
}


// class Breakpoints

static const char *BreakpointSetCodes[] = {
	"set_bp",
	"set_watchpoint read_mem",
	"set_watchpoint write_mem",
	"set_watchpoint read_io",
	"set_watchpoint write_io",
	"set_condition"
};

static QString getNextArgument(QString& data, int& pos)
{
	QString result;
	while (data[pos] == ' ' || data[pos] == '\t') ++pos;
	while (true) {
		if (data[pos] == ' ' || data[pos] == '\t') break;
		if (data[pos] == '}' || data[pos] == ']') break;
		result += data[pos];
		++pos;
	}
	return result;
}


bool Breakpoint::operator==(const Breakpoint &bp) const
{
	if (bp.type != type) return false;

	// compare address
	if (type != Breakpoint::CONDITION) {
		if (bp.range.start != range.start) return false;
		if (type != Breakpoint::BREAKPOINT) {
			if (bp.range.end != range.end) return false;
		}
		// compare slot
		if (bp.slot.ps != slot.ps || bp.slot.ss != slot.ss || bp.segment != segment) return false;
	}
	// compare condition
	return bp.condition == condition;
}

void Breakpoints::clear()
{
	breakpoints.clear();
}

void Breakpoints::setMemoryLayout(MemoryLayout* ml)
{
	memLayout = ml;
}

QString Breakpoints::createSetCommand(Breakpoint::Type type, std::optional<AddressRange> range,
                                      Slot slot, std::optional<uint8_t> segment,
                                      QString condition)
{
	QString cmd("debug %1 %2 %3");
	QString addr, cond;
	condition = condition.trimmed();

	if (type == Breakpoint::CONDITION) {
		// conditions don't have an address
		cond = QString("{%1}").arg(condition);
	} else if (range) {
		if (type == Breakpoint::BREAKPOINT || (range->end < range->start)) {
			// breakpoint (these never have a range) or watchpoint without range
			addr = QString::number(range->start);
		} else {
			// some type of watchpoint (with a valid range)
			addr = QString("{%1 %2}").arg(range->start).arg(*range->end);
		}

		cond = QString("{ [ %1_in_slot %2 %3 %4 ] %5}")
		       .arg(type == Breakpoint::WATCHPOINT_MEMREAD
		         || type == Breakpoint::WATCHPOINT_MEMWRITE ? "watch" : "pc")
		       .arg(slot.ps ? QString::number(*slot.ps) : "X")
		       .arg(slot.ss ? QString::number(*slot.ss) : "X")
		       .arg(segment ? QString::number(*segment) : "X")
		       .arg(condition.isEmpty() ? QString() : QString("&& ( %1 ) ").arg(condition));
	}
	return cmd.arg(BreakpointSetCodes[type])
	          .arg(addr)
	          .arg(escapeXML(cond));
}

QString Breakpoints::createRemoveCommand(const QString& id)
{
	// find breakpoint
	QString cmd("debug remove_");
	if (id.startsWith("bp"))
		cmd += "bp ";
	else if (id.startsWith("wp"))
		cmd += "watchpoint ";
	else
		cmd += "condition ";
	cmd += id;
	return cmd;
}

void Breakpoints::setBreakpoints(const QString& str)
{
	breakpoints.clear();

	QStringList bps = str.split('\n');
	for (auto& bp : bps) {
		if (bp.trimmed().isEmpty()) continue;

		Breakpoint newBp;

		// set id
		int p = 0;
		newBp.id = getNextArgument(bp, p);

		// determine type
		if (bp.startsWith("bp#")) {
			newBp.type = Breakpoint::BREAKPOINT;
		} else if (bp.startsWith("wp#")) {
			// determine watchpoint type
			static const char* const WpTypeNames[] = {"read_mem", "write_mem", "read_io", "write_io"};
			QString wpType = getNextArgument(bp, p);
			if (auto it = ranges::find(WpTypeNames, wpType); it != std::end(WpTypeNames)) {
				newBp.type = static_cast<Breakpoint::Type>(std::distance(WpTypeNames, it) + 1);
			} else {
				qWarning() << "Unknown" << wpType << "watchpoint type";
				continue;
			}
		} else if (bp.startsWith("cond#")) {
			newBp.type = Breakpoint::CONDITION;
		} else { // unknown
			continue;
		}

		// get address
		p++;
		if (newBp.type != Breakpoint::CONDITION) {
			if (bp[p] == '{') {
				p++;
				auto s = getNextArgument(bp, p);
				auto start = stringToValue<uint16_t>(s);
				if (!start) {
					qWarning() << "watchpoint starting range invalid:" << s;
					newBp.range = {};
					continue;
				}
				newBp.range.start = *start;
				int q = bp.indexOf('}', p);
				auto end = stringToValue<uint16_t>(bp.mid(p, q - p));
				newBp.range.end = end;
				p = q + 1;
			} else {
				auto s = getNextArgument(bp, p);
				auto start = stringToValue<uint16_t>(s);
				if (!start) {
					qWarning() << "breakpoint starting range invalid:" << s;
					newBp.range = {};
				}
				newBp.range.start = *start;
				newBp.range.end = {};
			}
		}
		// check and clip command (skip non-default commands)
		int q = bp.lastIndexOf('{');
		if (bp.mid(q).simplified() != "{debug break}") continue;

		newBp.condition = unescapeXML(bp.mid(p, q - p).trimmed());
		parseCondition(newBp);
		insertBreakpoint(newBp);
	}
}

QString Breakpoints::mergeBreakpoints(const QString& str)
{
	// copy breakpoints
	auto oldBps = breakpoints;
	// parse new list
	setBreakpoints(str);
	// check old list against new one
	QStringList mergeSet;
	for (const auto& old : oldBps) {
		auto newit = breakpoints.begin();
		for (/**/; newit != breakpoints.end(); ++newit) {
			// check for identical data
			if (old  == *newit) break;
		}
		if (newit == breakpoints.end()) {
			// create command to set this breakpoint again
			QString cmd = createSetCommand(old.type, old.range, old.slot, old.segment, old.condition);
			mergeSet << cmd;
		}
	}
	return mergeSet.join(" ; ");
}

void Breakpoints::parseCondition(Breakpoint& bp)
{
	bp.slot = {};
	bp.segment = {};

	// first split off braces
	if (bp.condition[0] == '{' && bp.condition.endsWith('}')) {
		if (bp.type != Breakpoint::CONDITION) {
			// check for slot argument
			QRegExp rx(R"(^\{\s*\[\s*(pc|watch)_in_slot\s([X0123])\s([X0123])\s(X|\d{1,3})\s*\]\s*(&&\s*\((.+)\)\s*)?\}$)");
			if (rx.indexIn(bp.condition) == 0) {
				bp.slot.ps = stringToValue<uint8_t>(rx.cap(2));
				bp.slot.ss = stringToValue<uint8_t>(rx.cap(3));
				bp.segment = stringToValue<uint8_t>(rx.cap(4));
				bp.condition = rx.cap(6).trimmed();
			} else {
				bp.condition.chop(1);
				bp.condition = bp.condition.mid(1).trimmed();
			}
		} else {
			bp.condition.chop(1);
			bp.condition = bp.condition.mid(1).trimmed();
		}
	}
}

bool Breakpoints::inCurrentSlot(const Breakpoint& bp)
{
	if (!memLayout) return true;

	int page = (bp.range.start & 0xC000) >> 14;
	if (!bp.slot.ps || *bp.slot.ps == memLayout->primarySlot[page]) {
		if (memLayout->isSubslotted[*bp.slot.ps & 3]) {
			if (!bp.slot.ss || *bp.slot.ss == memLayout->secondarySlot[page]) {
				if (memLayout->mapperSize[*bp.slot.ps & 3][*bp.slot.ss & 3] > 0) {
					if (bp.segment || *bp.segment == memLayout->mapperSegment[page]) {
						return true;
					}
				} else {
					return true;
				}
			}
		} else if (bp.slot.ss) {
			return false;
		} else if (memLayout->mapperSize[*bp.slot.ps & 3][0] > 0) {
			if (!bp.segment || *bp.segment == memLayout->mapperSegment[page]) {
				return true;
			}
		} else {
			return true;
		}
	}
	return false;
}

void Breakpoints::insertBreakpoint(Breakpoint& bp)
{
	auto it = ranges::upper_bound(breakpoints, bp.range,
	                              [](auto& bp1, auto& bp2) { return bp1.start < bp2.start; },
	                              &Breakpoint::range);
	breakpoints.insert(it, bp);
}

int Breakpoints::breakpointCount()
{
	return breakpoints.size();
}

bool Breakpoints::isBreakpoint(quint16 addr, QString* id, bool checkSlot)
{
	if (std::optional<uint16_t> index = findBreakpoint(addr)) {
		do {
			const auto& bp = breakpoints[*index];
			if ((!checkSlot || inCurrentSlot(bp)) && bp.type == Breakpoint::BREAKPOINT) {
				if (id) *id = bp.id;
				return true;
			}
		} while (breakpoints[++(*index)].range.start == addr);
	}
	return false;
}

bool Breakpoints::isWatchpoint(quint16 addr, QString* id, bool checkSlot)
{
	for (const auto& bp : breakpoints) {
		if (bp.type == Breakpoint::WATCHPOINT_MEMREAD || bp.type == Breakpoint::WATCHPOINT_MEMWRITE) {
			if ((bp.range.start == addr && !bp.range.end) ||
			    (addr >= bp.range.start && addr <= bp.range.end)) {
				if (!checkSlot || inCurrentSlot(bp)) {
					if (id) *id = bp.id;
					return true;
				}
			}
		}
	}
	return false;
}

std::optional<uint16_t>
Breakpoints::findBreakpoint(uint16_t addr)
{
        if (auto i = ranges::lower_bound(breakpoints, addr,
            [](auto& bp1, auto& addr) { return bp1.start < addr; }, &Breakpoint::range);
	    i != breakpoints.end() && i->range.start == addr) {
		return std::distance(breakpoints.begin(), i);
	}
	return {};
}

const Breakpoint& Breakpoints::getBreakpoint(int index)
{
	assert((long unsigned) index < breakpoints.size());
	return breakpoints[index];
}

void Breakpoints::saveBreakpoints(QXmlStreamWriter& xml)
{
	// write symbols
	for (const auto& bp : breakpoints) {
		xml.writeStartElement("Breakpoint");

		// type
		xml.writeAttribute("type", TypeNames[bp.type]);

		// id
		xml.writeAttribute("id", bp.id);

		// slot/segment
		xml.writeAttribute("primarySlot", QString(bp.slot.ps ? QChar('0' + *bp.slot.ps) : '*'));
		xml.writeAttribute("secondarySlot", QString(bp.slot.ss ? QChar('0' + *bp.slot.ss) : '*'));
		xml.writeAttribute("segment", QString::number(*bp.segment));

		// address
		if (bp.type == Breakpoint::BREAKPOINT) {
			xml.writeTextElement("address", QString::number(bp.range.start));
		} else if (bp.type != Breakpoint::CONDITION) {
			xml.writeTextElement("regionStart", QString::number(bp.range.start));
			if (bp.range.end) xml.writeTextElement("regionEnd", QString::number(*bp.range.end));
		}

		// condition
		xml.writeTextElement("condition", bp.condition);

		// complete
		xml.writeEndElement();
	}
}

void Breakpoints::loadBreakpoints(QXmlStreamReader& xml)
{
	Breakpoint bp;
	while (!xml.atEnd()) {
		xml.readNext();
		// exit if closing of main tag
		if (xml.isEndElement()) {
			if (xml.name() == "Breakpoints") {
				break;
			} else if (xml.name() == "Breakpoint") {
				insertBreakpoint(bp);
			}
		}
		// begin tag
		if (xml.isStartElement()) {
			if (xml.name() == "Breakpoint") {
				// set type
				QString label = xml.attributes().value("type").toString().toLower();
				if (auto it = ranges::find(TypeNames, label); it != std::end(TypeNames)) {
					bp.type = static_cast<Breakpoint::Type>(std::distance(TypeNames, it));
				} else {
					qWarning() << "Unknown type" << label << "in XML";
					bp.type = Breakpoint::BREAKPOINT;
				}

				// id
				bp.id = xml.attributes().value("id").toString();

				// slot/segment
				char c = xml.attributes().value("primarySlot").at(0).toLatin1();
				bp.slot.ps = make_if(c >= '0' && c <= '3', c - '0');
				c = xml.attributes().value("secondarySlot").at(0).toLatin1();
				bp.slot.ss = make_if(c >= '0' && c <= '3', c - '0');
				bp.segment = stringToValue<uint8_t>(xml.attributes().value("segment")
						.toString());

			} else if (xml.name() == "address" || xml.name() == "regionStart") {
				// read symbol name
				auto s = xml.readElementText();
				auto start = stringToValue<uint16_t>(s);
				if (!start) {
					qWarning() << "breakpoint starting range invalid:" << s;
					continue;
				}
				bp.range.start = *start;
				if (bp.type == Breakpoint::BREAKPOINT) bp.range.end = {};

			} else if (xml.name() == "regionEnd") {
				// read symbol name
				bp.range.end = stringToValue<uint16_t>(xml.readElementText());
			} else if (xml.name() == "condition") {
				bp.condition = xml.readElementText().trimmed();
			}
		}
	}
}
