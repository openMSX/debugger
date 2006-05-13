// $Id$

#include "DebuggerData.h"

#include <QStringList>


//
// MemoryLayout
//

MemoryLayout::MemoryLayout()
{
	// init
	for(int p=0; p<4; p++) {
		primarySlot[p] = '0';
		secondarySlot[p] = 'X';
		mapperSegment[p] = 0;
		isSubslotted[p] = FALSE;
		for(int q=0; q<4; q++)
			mapperSize[p][q] = 0;
	}
}

//
// Breakpoints
//

void Breakpoints::setMemoryLayout(MemoryLayout *ml)
{
	memLayout = ml;
}

void Breakpoints::setBreakpoints(const QString& str)
{
	QStringList bps = str.split('\n');
	
	breakpoints.clear();
	for (QStringList::Iterator it = bps.begin(); it != bps.end(); ++it) {
		Breakpoint newBp;
		int p = (*it).indexOf(' ');
		newBp.id = (*it).left(p).trimmed();
		if(newBp.id.isEmpty()) break;
		int q = (*it).indexOf(' ', p+1);
		if(q==-1) {
			newBp.address = (*it).mid(p).toUShort(0,0);
		} else {
			newBp.address = (*it).mid(p, q-p).trimmed().toUShort(0,0);
			newBp.condition = (*it).mid(q).trimmed();
		}
		parseCondition(newBp);
		insertBreakpoint(newBp);
	}
}

void Breakpoints::parseCondition(Breakpoint& bp)
{
	int p = bp.condition.indexOf("pc_in_slot ");
	
	bp.ps = '*';
	bp.ss = '*';
	bp.segment = -1;
	
	if(p>=0) {
		p += 11;
		QString arg = getNextArgument(bp.condition, p);
		if(!arg.isEmpty()) {
			int s = arg.toInt();
			if(s<0 || s>3) return;
			bp.ps = '0'+s;
			arg = getNextArgument(bp.condition, p);
			if(!arg.isEmpty()) {
				s = arg.toInt();
				if(s<0 || s>3) return;
				bp.ss = '0'+s;
				arg = getNextArgument(bp.condition, p);
				if(!arg.isEmpty()) {
					s = arg.toInt();
					if(s<0 || s>255) return;
					bp.segment = s;
				}
			}
		}
	}
}

QString Breakpoints::getNextArgument(QString& data, int& pos)
{
	QString result;
	
	while(data[pos]==' ' || data[pos]=='\t') pos++;
	for(;;) {
		if(data[pos]==' ' || data[pos]=='\t') break;
		if(data[pos]=='}' || data[pos]==']') break;
		result += data[pos];
		pos++;
	}
	
	return result;
}

bool Breakpoints::inCurrentSlot(const Breakpoint& bp)
{
	if(memLayout==NULL) return TRUE;
	
	int page = (bp.address & 0xC000) >> 14;
	if(bp.ps=='*' || bp.ps==memLayout->primarySlot[page]) {
		if(memLayout->isSubslotted[bp.ps & 3]) {
			if(bp.ss=='*' || bp.ss==memLayout->secondarySlot[page]) {
				if(memLayout->mapperSize[bp.ps & 3][bp.ss & 3]>0) {
					if(bp.segment==memLayout->mapperSegment[page])
						return TRUE;
				} else
					return TRUE;
			}
		} else if(memLayout->mapperSize[bp.ps & 3][0]>0) {
			if(bp.segment==memLayout->mapperSegment[page])
				return TRUE;
		} else
			return TRUE;
	}
	return FALSE;
}

void Breakpoints::insertBreakpoint(Breakpoint& bp)
{
	BreakpointList::iterator it;
	
	for(it = breakpoints.begin(); it != breakpoints.end(); ++it)
		if( it->address > bp.address) {
			breakpoints.insert(it, bp);
			return;
		};

	breakpoints.append(bp);
}

int Breakpoints::breakpointCount()
{
	return breakpoints.size();
}

bool Breakpoints::isBreakpoint(quint16 addr)
{
	for(BreakpointList::const_iterator it = breakpoints.constBegin(); it != breakpoints.constEnd(); ++it) {
		if(it->address == addr)
			if(inCurrentSlot(*it))
				return TRUE;
	}
	
	return FALSE;
}

const QString Breakpoints::idString(quint16 addr)
{
	for(BreakpointList::const_iterator it = breakpoints.constBegin(); it != breakpoints.constEnd(); ++it) {
		if(it->address == addr)
			if(inCurrentSlot(*it))
				return it->id;
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
