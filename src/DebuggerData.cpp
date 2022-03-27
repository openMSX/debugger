#include "DebuggerData.h"
#include "Convert.h"
#include "qglobal.h"
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
		if (bp.address != address) return false;
		if (type != Breakpoint::BREAKPOINT) {
			int re1 = -1, re2 = -1;
			if (bp.regionEnd != quint16(-1) && bp.regionEnd != bp.address) re1 = bp.regionEnd;
			if (   regionEnd != quint16(-1) &&    regionEnd !=    address) re2 =    regionEnd;
			if (re1 != re2) return false;
		}
		// compare slot
		if (bp.ps != ps || bp.ss != ss || bp.segment != segment) return false;
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

QString Breakpoints::createSetCommand(Breakpoint::Type type, quint16 address, qint8 ps, qint8 ss,
                                      qint16 segment, int endRange, QString condition)
{
	QString cmd("debug %1 %2 %3");
	QString addr, cond;
	condition = condition.trimmed();

	if (type == Breakpoint::CONDITION) {
		// conditions don't have an address
		cond = QString("{%1}").arg(condition);
	} else {
		if ((type == Breakpoint::BREAKPOINT) || (endRange < address)) {
			// breakpoint (these never have a range) or watchpoint without range
			addr = QString::number(address);
		} else {
			// some type of watchpoint (with a valid range)
			addr = QString("{%1 %2}").arg(address).arg(endRange);
		}

		cond = QString("{ [ %1_in_slot %2 %3 %4 ] %5}")
		       .arg(type == Breakpoint::WATCHPOINT_MEMREAD
		         || type == Breakpoint::WATCHPOINT_MEMWRITE ? "watch" : "pc")
		       .arg(ps == -1 ? 'X' : char('0' + ps))
		       .arg(ss == -1 ? 'X' : char('0' + ss))
		       .arg(segment == -1 ? QString('X') : QString::number(segment))
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
			static const char* const WpTypeNames[] = { "read_mem", "write_mem", "read_io", "write_io" };
			QString wptype = getNextArgument(bp, p);
			auto iter = std::find(std::begin(WpTypeNames), std::end(WpTypeNames), wptype);
			if (iter == std::end(WpTypeNames)) {
				qWarning() << "Unknown" << wptype << "watchpoint type";
				continue;
			}
			newBp.type = static_cast<Breakpoint::Type>(std::distance(WpTypeNames, iter) + 1);
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
				newBp.address = stringToValue(getNextArgument(bp, p));
				int q = bp.indexOf('}', p);
				newBp.regionEnd = stringToValue(bp.mid(p, q - p));
				p = q + 1;
			} else {
				newBp.address = stringToValue(getNextArgument(bp, p));
				newBp.regionEnd = newBp.address;
			}
		} else {
			newBp.address = -1;
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
			QString cmd = createSetCommand(old.type, old.address, old.ps, old.ss, old.segment,
			                               old.regionEnd, old.condition);
			mergeSet << cmd;
		}
	}
	return mergeSet.join(" ; ");
}

void Breakpoints::parseCondition(Breakpoint& bp)
{
	bp.ps = -1;
	bp.ss = -1;
	bp.segment = -1;

	// first split off braces
	if (bp.condition[0] == '{' && bp.condition.endsWith('}')) {
		if (bp.type != Breakpoint::CONDITION) {
			// check for slot argument
			QRegExp rx("^\\{\\s*\\[\\s*(pc|watch)_in_slot\\s([X0123])\\s([X0123])\\s(X|\\d{1,3})\\s*\\]\\s*(&&\\s*\\((.+)\\)\\s*)?\\}$");
			if (rx.indexIn(bp.condition) == 0) {
				bool ok;
				bp.ps = rx.cap(2).toInt(&ok);
				if (!ok) bp.ps = -1;
				bp.ss = rx.cap(3).toInt(&ok);
				if (!ok) bp.ss = -1;
				bp.segment = rx.cap(4).toInt(&ok);
				if (!ok) bp.segment = -1;
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

	int page = (bp.address & 0xC000) >> 14;
	if (bp.ps == -1 || bp.ps == memLayout->primarySlot[page]) {
		if (memLayout->isSubslotted[bp.ps & 3]) {
			if (bp.ss == -1 || bp.ss == memLayout->secondarySlot[page]) {
				if (memLayout->mapperSize[bp.ps & 3][bp.ss & 3] > 0) {
					if (bp.segment == -1 || bp.segment == memLayout->mapperSegment[page]) {
						return true;
					}
				} else {
					return true;
				}
			}
		} else if (bp.ss != -1) {
			return false;
		} else if (memLayout->mapperSize[bp.ps & 3][0] > 0) {
			if (bp.segment == -1 || bp.segment == memLayout->mapperSegment[page]) {
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
	auto it = std::upper_bound(breakpoints.begin(), breakpoints.end(), bp,
	    [](const Breakpoint& bp1, const Breakpoint& bp2) {
	        return bp1.address < bp2.address;
	    }
	);
	breakpoints.insert(it, bp);
}

int Breakpoints::breakpointCount()
{
	return breakpoints.size();
}

bool Breakpoints::isBreakpoint(quint16 addr, QString* id, bool checkSlot)
{
	if (int index = findBreakpoint(addr); index != -1) {
		do {
			const auto& bp = breakpoints[index];
			if ((!checkSlot || inCurrentSlot(bp)) && bp.type == Breakpoint::BREAKPOINT) {
				if (id) *id = bp.id;
				return true;
			}
		} while (breakpoints[++index].address == addr);
	}
	return false;
}

bool Breakpoints::isWatchpoint(quint16 addr, QString* id, bool checkSlot)
{
	for (const auto& bp : breakpoints) {
		if (bp.type == Breakpoint::WATCHPOINT_MEMREAD || bp.type == Breakpoint::WATCHPOINT_MEMWRITE) {
			if ((bp.address == addr && bp.regionEnd < bp.address) ||
			    (addr >= bp.address && addr <= bp.regionEnd)) {
				if (!checkSlot || inCurrentSlot(bp)) {
					if (id) *id = bp.id;
					return true;
				}
			}
		}
	}
	return false;
}

int Breakpoints::findBreakpoint(quint16 addr)
{
	auto i = std::lower_bound(breakpoints.begin(), breakpoints.end(), addr,
	                [](const Breakpoint& bp, const quint16 addr) { return bp.address < addr; });
	if (i != breakpoints.end() && i->address == addr) {
		return std::distance(breakpoints.begin(), i);
	}
	return -1;
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
		xml.writeAttribute("primarySlot", QString(bp.ps < 0 ? '*' : char('0' + bp.ps)));
		xml.writeAttribute("secondarySlot", QString(bp.ss < 0 ? '*' : char('0' + bp.ss)));
		xml.writeAttribute("segment", QString::number(bp.segment));

		// address
		if (bp.type == Breakpoint::BREAKPOINT) {
			xml.writeTextElement("address", QString::number(bp.address));
		} else if (bp.type != Breakpoint::CONDITION) {
			xml.writeTextElement("regionStart", QString::number(bp.address));
			xml.writeTextElement("regionEnd", QString::number(bp.regionEnd));
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
				auto iter = std::find(std::begin(TypeNames), std::end(TypeNames), label);
				if (iter == std::end(TypeNames)) {
					qWarning() << "Unknown type" << label << "in XML";
					bp.type = Breakpoint::BREAKPOINT;
				} else {
					bp.type = static_cast<Breakpoint::Type>(std::distance(TypeNames, iter));
				}

				// id
				bp.id = xml.attributes().value("id").toString();

				// slot/segment
				char c = xml.attributes().value("primarySlot").at(0).toLatin1();
				bp.ps = c >= '0' && c <= '3' ? c - '0' : -1;
				c = xml.attributes().value("secondarySlot").at(0).toLatin1();
				bp.ss = c >= '0' && c <= '3' ? c - '0' : -1;
				bp.segment = xml.attributes().value("segment").toString().toInt();

			} else if (xml.name() == "address" || xml.name() == "regionStart") {
				// read symbol name
				bp.address = xml.readElementText().toInt();
				if (bp.type == Breakpoint::BREAKPOINT) bp.regionEnd = bp.address;

			} else if (xml.name() == "regionEnd") {
				// read symbol name
				bp.regionEnd = xml.readElementText().toInt();
			} else if (xml.name() == "condition") {
				bp.condition = xml.readElementText().trimmed();
			}
		}
	}
}
