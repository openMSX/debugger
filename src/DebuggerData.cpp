// $Id$

#include "DebuggerData.h"
#include <QStringList>
#include <QXmlStreamWriter>


//
// MemoryLayout
//

MemoryLayout::MemoryLayout()
{
	for (int p = 0; p < 4; ++p) {
		primarySlot[p] = '0';
		secondarySlot[p] = 'X';
		mapperSegment[p] = 0;
		isSubslotted[p] = false;
		for (int q = 0; q < 4; ++q) {
			mapperSize[p][q] = 0;
		}
	}
}

//
// Breakpoints
//

Breakpoints::Breakpoints()
	: memLayout(NULL)
{
}

void Breakpoints::clear()
{
	breakpoints.clear();
}

void Breakpoints::setMemoryLayout(MemoryLayout* ml)
{
	memLayout = ml;
}

void Breakpoints::setBreakpoints(const QString& str)
{
	QStringList bps = str.split('\n');

	breakpoints.clear();
	for (QStringList::Iterator it = bps.begin(); it != bps.end(); ++it) {
		Breakpoint newBp;
		newBp.type = BREAKPOINT;
		int p = it->indexOf(' ');
		newBp.id = it->left(p).trimmed();
		if(newBp.id.isEmpty()) break;
		int q = it->indexOf(' ', p+1);
		if (q == -1) {
			newBp.address = it->mid(p).toUShort(0,0);
		} else {
			newBp.address = it->mid(p, q-p).trimmed().toUShort(0,0);
			newBp.condition = it->mid(q).trimmed();
		}
		newBp.regionEnd = newBp.address;
		parseCondition(newBp);
		insertBreakpoint(newBp);
	}
}

QString Breakpoints::mergeBreakpoints(const QString& str)
{
	// copy breakpoints
	BreakpointList oldBps( breakpoints );
	// parse new list
	setBreakpoints( str );
	// check old list against new one
	QStringList mergeSet;
	while( oldBps.size() ) {
		Breakpoint& old = oldBps.first();
		BreakpointList::iterator newit = breakpoints.begin();
		while( newit != breakpoints.end() ) {
			// check for identical location
			if( old.type == newit->type &&
			    old.address == newit->address &&
			    old.regionEnd == newit->regionEnd &&
			    old.ps == newit->ps &&
			    old.ss == newit->ss &&
			    old.segment == newit->segment )
			{
				break;
			}
			newit++;
		}
		if( newit == breakpoints.end() ) {
			// create command to set this breakpoint again
			QString cmd;
			cmd.sprintf("debug set_bp %i { [ pc_in_slot %c %c %i ] }",
			            old.address, old.ps, old.ss, old.segment );
			mergeSet << cmd;
		}
		oldBps.removeFirst();
	}
	return mergeSet.join(" ; ");
}

static QString getNextArgument(QString& data, int& pos)
{
	QString result;
	while (data[pos] == ' ' || data[pos]=='\t') ++pos;
	while (true) {
		if (data[pos] == ' ' || data[pos] == '\t') break;
		if (data[pos] == '}' || data[pos] == ']') break;
		result += data[pos];
		++pos;
	}
	return result;
}

void Breakpoints::parseCondition(Breakpoint& bp)
{
	bp.ps = '*';
	bp.ss = '*';
	bp.segment = -1;

	int p = bp.condition.indexOf("pc_in_slot ");
	if (p >= 0) {
		p += 11;
		QString arg = getNextArgument(bp.condition, p);
		if (!arg.isEmpty()) {
			int s = arg.toInt();
			if (s < 0 || s > 3) return;
			bp.ps = '0' + s;
			arg = getNextArgument(bp.condition, p);
			if (!arg.isEmpty()) {
				s = arg.toInt();
				if (s < 0 || s > 3) return;
				bp.ss = '0' + s;
				arg = getNextArgument(bp.condition, p);
				if (!arg.isEmpty()) {
					s = arg.toInt();
					if (s < 0 || s > 255) return;
					bp.segment = s;
				}
			}
		}
	}
}

bool Breakpoints::inCurrentSlot(const Breakpoint& bp)
{
	if (memLayout == NULL) return true;
	
	int page = (bp.address & 0xC000) >> 14;
	if (bp.ps == '*' || bp.ps == memLayout->primarySlot[page]) {
		if (memLayout->isSubslotted[bp.ps & 3]) {
			if (bp.ss == '*' || bp.ss == memLayout->secondarySlot[page]) {
				if (memLayout->mapperSize[bp.ps & 3][bp.ss & 3] > 0) {
					if (bp.segment == memLayout->mapperSegment[page]) {
						return true;
					}
				} else {
					return true;
				}
			}
		} else if (memLayout->mapperSize[bp.ps & 3][0] > 0) {
			if (bp.segment == memLayout->mapperSegment[page]) {
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
	for (BreakpointList::iterator it = breakpoints.begin();
	     it != breakpoints.end(); ++it) {
		if (it->address > bp.address) {
			breakpoints.insert(it, bp);
			return;
		}
	}
	breakpoints.append(bp);
}

int Breakpoints::breakpointCount()
{
	return breakpoints.size();
}

bool Breakpoints::isBreakpoint(quint16 addr)
{
	for (BreakpointList::const_iterator it = breakpoints.constBegin();
	     it != breakpoints.constEnd(); ++it) {
		if (it->address == addr) {
			if (inCurrentSlot(*it)) {
				return true;
			}
		}
	}
	return false;
}

const QString Breakpoints::idString(quint16 addr)
{
	for (BreakpointList::const_iterator it = breakpoints.constBegin();
	     it != breakpoints.constEnd(); ++it) {
		if (it->address == addr) {
			if (inCurrentSlot(*it)) {
				return it->id;
			}
		}
	}
	return "";
}

int Breakpoints::findBreakpoint(quint16 addr)
{
	// stub
	// will implement findfirst/findnext scheme for speed
	return addr;
}

int Breakpoints::findNextBreakpoint()
{
	// stub
	// will implement findfirst/findnext scheme for speed
	return -1;
}


void Breakpoints::saveBreakpoints( QXmlStreamWriter& xml )
{
	// write symbols
	BreakpointList::iterator it = breakpoints.begin();
	while( it != breakpoints.end() ) {
		xml.writeStartElement("Breakpoint");
		// type
		switch( it->type ) {
			case BREAKPOINT:
				xml.writeAttribute("type", "breakpoint");
				break;
			case WATCHPOINT_IOREAD:
				xml.writeAttribute("type", "ioread");
				break;
			case WATCHPOINT_IOWRITE:
				xml.writeAttribute("type", "iowrite");
				break;
			case WATCHPOINT_MEMREAD:
				xml.writeAttribute("type", "memread");
				break;
			case WATCHPOINT_MEMWRITE:
				xml.writeAttribute("type", "memwrite");
				break;
		}
		// id
		xml.writeAttribute("id", it->id);
		// slot/segment
		xml.writeAttribute("primarySlot", QString(it->ps) );
		xml.writeAttribute("secondarySlot", QString(it->ss) );
		xml.writeAttribute("segment", QString::number(it->segment) );
		// address
		if( it->type == BREAKPOINT ) {
			xml.writeTextElement("address", QString::number(it->address) );
		} else {
			xml.writeTextElement("regionStart", QString::number(it->address) );
			xml.writeTextElement("regionEnd", QString::number(it->regionEnd) );
		}
		// condition not supported yet
		// complete
		xml.writeEndElement();
		it++;
	}
}

void Breakpoints::loadBreakpoints( QXmlStreamReader& xml )
{
	Breakpoint bp;
	while( !xml.atEnd() ) {
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
		if( xml.isStartElement() ) {
			if( xml.name() == "Breakpoint" ) {

				// set type
				QString type = xml.attributes().value("type").toString().toLower();
				if( type == "ioread" )
					bp.type = WATCHPOINT_IOREAD;
				else if( type == "iowrite" )
					bp.type = WATCHPOINT_IOWRITE;
				else if( type == "memread" )
					bp.type = WATCHPOINT_MEMREAD;
				else if( type == "memwrite" )
					bp.type = WATCHPOINT_MEMWRITE;
				else
					bp.type = BREAKPOINT;
				// id
				bp.id = xml.attributes().value("id").toString();
				// slot/segment
				bp.ps = xml.attributes().value("primarySlot").at(0).toAscii();
				bp.ss = xml.attributes().value("secondarySlot").at(0).toAscii();
				bp.segment = xml.attributes().value("segment").toString().toInt();
				
			} else if( xml.name() == "address" || xml.name() == "regionStart" ) {
				
				// read symbol name
				bp.address = xml.readElementText().toInt();
				if( bp.type == BREAKPOINT ) bp.regionEnd = bp.address;
				
			} else if( xml.name() == "regionEnd" ) {
				
				// read symbol name
				bp.regionEnd = xml.readElementText().toInt();
				
			}
		}
	}
}
