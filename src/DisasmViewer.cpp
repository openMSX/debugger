// $Id$

#include "DisasmViewer.h"
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionFocusRect>
#include <cmath>
#include <cassert>

DisasmViewer::DisasmViewer( QWidget* parent )
	: QFrame( parent )
{
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);

	setFont(QFont( "Verdana", 11));
	
	breakMarker = QPixmap(":/icons/breakpoint.png");
	pcMarker = QPixmap(":/icons/pcarrow.png");
	
	memory = NULL;
	cursorAddr = (quint16)-1;
	programCounter = 0x1070;
	waitingForData = FALSE;
	nextRequest = NULL;
	
	scrollBar = new QScrollBar(Qt::Vertical, this);
	scrollBar->setMinimum(0);
	scrollBar->setMaximum(0xFFFF);
	scrollBar->setSingleStep(0);
	scrollBar->setPageStep(0);

	frameL = frameT = frameB = frameWidth();
	frameR = frameL + scrollBar->sizeHint().width();

	visibleLines = double(height() - frameT - frameB) / fontMetrics().height();

	// manual scrollbar handling routines (real size of the data is not known)
	connect(scrollBar, SIGNAL( actionTriggered(int) ), this, SLOT( scrollBarAction(int) ) );
	connect(scrollBar, SIGNAL( valueChanged(int) ), this, SLOT( scrollBarChanged(int) ) );

}

void DisasmViewer::resizeEvent(QResizeEvent *e)
{
	QFrame::resizeEvent(e);
	
	scrollBar->setGeometry(width() - frameR, frameT,
	                       scrollBar->sizeHint().width(),
	                       height()-frameT-frameB);
	scrollBar->setMaximum(0x10000-int(visibleLines));
	
	// calc the number of lines that can be displayed
	// partial lines count as a whole
	visibleLines = double(height() - frameT - frameB) / fontMetrics().height();
	// reset the address in order to trigger a check on the disasmLines
	setAddress(scrollBar->value());
}

void DisasmViewer::paintEvent(QPaintEvent *e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);
	
	QPainter p(this);
	int h = fontMetrics().height();
	int d = fontMetrics().descent();
	
	// calc and set drawing bounds
	QRect r(e->rect());
	if(r.left()<frameL) r.setLeft(frameL);
	if(r.top()<frameT) r.setTop(frameT);
	if(r.right()>width()-frameR-1) r.setRight(width()-frameR-1);
	if(r.bottom()>height()-frameB-1) r.setBottom(height()-frameB-1);
	p.setClipRect(r);

	// draw icon column first
	p.fillRect(frameL, frameT, 32, height()-frameT-frameB, QColor(224, 224, 224));
	
	// calc layout (not optimal)
	int charWidth = fontMetrics().width("MULUW") / 5;
	int xAddr = frameL + 40;
	int xMCode[4];
	xMCode[0] = xAddr + 6*charWidth;
	xMCode[1] = xMCode[0] + 3*charWidth;
	xMCode[2] = xMCode[1] + 3*charWidth;
	xMCode[3] = xMCode[2] + 3*charWidth;
	int xMnem   = xMCode[3] + 4*charWidth;
	int xMnemArg = xMnem + 8*charWidth;

	int y = frameT + h - 1;
	
	QString hexStr;
	const DisasmRow *row;
	bool displayDisasm = memory!=NULL && isEnabled();

	for(int i=0; i<int(ceil(visibleLines)); i++) {

		// default to text pen 
		p.setPen( palette().color(QPalette::Text) );

		// fetch the data for this line
		if(displayDisasm)
			row = &disasmLines[i+disasmTopLine];
		else
			row = &DISABLED_ROW;

		// draw cursor line or breakpoint
		if(row->addr==cursorAddr) {
			p.fillRect(frameL+32, y+1-h, 
			           width() -32 - frameL - frameR ,h, palette().color(QPalette::Highlight));
			if(hasFocus()) {
				QStyleOptionFocusRect so;
				so.backgroundColor = palette().color(QPalette::Highlight);
				so.rect = QRect(frameL+32, y+1-h, width()-32-frameL-frameR ,h);
				style()->drawPrimitive(QStyle::PE_FrameFocusRect, &so, &p, this);
			}
			p.setPen(palette().color(QPalette::HighlightedText));
		}
		if( breakpoints->isBreakpoint(row->addr) ) {
			// draw breakpoint
			p.drawPixmap(frameL + 2, y - h/2 -5, breakMarker);
			if(row->addr!=cursorAddr) {
				p.fillRect(frameL+32, y+1-h, 
				           width() -32 - frameL - frameR ,h, Qt::red);
				p.setPen(Qt::white);
			}
		}
		// check for PC
		if(row->addr==programCounter) {
			p.drawPixmap(frameL + 18, y - h/2 -5, pcMarker);
		}

		// print the address
		hexStr.sprintf("%04X", row->addr);
		p.drawText(xAddr, y-d, hexStr);

		// print 1 to 4 bytes
		for(int j=0; j<row->numBytes; j++){
			hexStr.sprintf("%02X", displayDisasm ? memory[row->addr + j] : 0);
			p.drawText(xMCode[j], y-d, hexStr);
		}

		// print the instruction and arguments
		p.drawText(xMnem, y-d, row->instr.substr(0, 7).c_str());
		p.drawText(xMnemArg, y-d, row->instr.substr(7, 20).c_str());

		// check for end of memory to avoid drawing the last (partial) line
		if(int(row->addr)+row->numBytes>0xFFFF) break;
		y += h;
	}
}

void DisasmViewer::setAddress(quint16 addr, int method, bool doRepaint)
{
	int line = findDisasmLine(addr);

	if(line>=0) {
		int dt, db;
		switch(method) {
			case Top:
			case Middle:
			case Bottom:
			case Closest:
				dt = db = 10;
				break;
			case TopAlways:
				dt = 10;
				db = int(visibleLines)+10;
				break;
			case MiddleAlways:
				dt = db = 10+int(visibleLines)/2;
				break;
			case BottomAlways:
				dt = 10+int(visibleLines);
				db = 10;
				break;
			default:
				assert(false);
				dt = db = 0; // avoid warning
		}
	
		if( (line>dt || disasmLines[0].addr==0) 
			&& (line<int(disasmLines.size())-db || disasmLines.back().addr+disasmLines.back().numBytes==0x10000 ))
		{
			// line is within existing disasm'd data. Find where to put it.
			if(method==Top || method==TopAlways || 
				(method==Closest && addr<disasmLines[disasmTopLine].addr ))
			{
				// Move line to top
				disasmTopLine = line;
			} 
			else if(method==Bottom || method==BottomAlways || 
				(method==Closest && addr>=disasmLines[disasmTopLine+int(visibleLines)-1].addr ))
			{
				// Move line to bottom
				disasmTopLine = line-int(visibleLines)+1;
			} 
			else if(method==MiddleAlways || (method=Middle && addr<disasmLines[disasmTopLine].addr
				&& addr>disasmLines[disasmTopLine+int(visibleLines)-1].addr))
			{
				// Move line to middle
				disasmTopLine = line-int(visibleLines)/2;
			}
			if(disasmTopLine<0) disasmTopLine = 0;
			if(disasmTopLine>=int(disasmLines.size())) disasmTopLine = disasmLines.size()-1;
			if(doRepaint) update();
			return;
		}
	}

	if(method==Closest)
		if(addr<disasmLines[disasmTopLine].addr) {
			method = Top;
		} else {
			method = Bottom;
		}
	

	// The requested address it outside the pre-disassembled bounds.
	// This means that a new block of memory must be transfered from
	// openMSX and disassembled.

	// determine disasm bounds
	int disasmStart;
	int disasmEnd;
	int extra = 4*(int(visibleLines)>9?int(ceil(visibleLines)):10);

	switch(method) {
		case Middle:
		case MiddleAlways:
			disasmStart = addr - 3*extra/2;
			disasmEnd = addr + 3*extra/2;
			break;
		case Bottom:
		case BottomAlways:
			disasmStart = addr - 2*extra;
			disasmEnd = addr + extra;
			break;
		default:
			disasmStart = addr - extra;
			disasmEnd = addr + 2*extra;
	}
	if(disasmStart<0) disasmStart = 0;
	if(disasmEnd>0xFFFF) disasmEnd = 0xFFFF;
	
	CommMemoryRequest *req = new CommMemoryRequest(disasmStart, disasmEnd-disasmStart+1, memory, disasmStart);
	req->address = addr;
	req->method = method;
	req->repaint = doRepaint;
	
	if(waitingForData) {
		if(nextRequest) delete nextRequest;
		nextRequest = req;
	} else {
		emit needUpdate(req);
		waitingForData = TRUE;
	}
}

void DisasmViewer::memoryUpdated(CommMemoryRequest *req)
{
	// disassemble the newly received memory
	dasm(memory, req->readOffset, req->readOffset+req->readSize-1, disasmLines);

	// locate the requested line 
	disasmTopLine = 0;
	while(disasmLines[disasmTopLine].addr<req->address
	      && int(disasmLines.size()-disasmTopLine)>int(visibleLines)) disasmTopLine++;
	
	switch(req->method) {
		case Middle:
		case MiddleAlways:
			disasmTopLine -= int(visibleLines)/2;
			break;
		case Bottom:
		case BottomAlways:
			disasmTopLine -= int(visibleLines)-1;
			break;
	}
	if(disasmTopLine<0) disasmTopLine = 0;
	
	// sync the scrollbar with the actual address reached
	if(!nextRequest)
		scrollBar->setValue(disasmLines[disasmTopLine].addr);

	if(req->repaint) update();
	updateCancelled(req);
}

void DisasmViewer::updateCancelled(CommMemoryRequest *req)
{
	delete req;
	if(nextRequest) {
		emit needUpdate(nextRequest);
		nextRequest = NULL;
	} else {
		waitingForData = FALSE;
	}
}

void DisasmViewer::setProgramCounter(quint16 pc)
{
	programCounter = pc;
	setAddress(pc, MiddleAlways);
}

int DisasmViewer::findDisasmLine(quint16 lineAddr)
{
	int line = disasmLines.size();
	
	while(--line>=0)
		if(disasmLines[line].addr == lineAddr) 
			break;

	return line;
}	

void DisasmViewer::scrollBarAction(int action)
{
	switch(action) {
		case QScrollBar::SliderSingleStepAdd:
			setAddress(disasmLines[disasmTopLine+1].addr, TopAlways);
			break;
		case QScrollBar::SliderSingleStepSub:
			if(disasmTopLine>0)
				setAddress(disasmLines[disasmTopLine-1].addr, TopAlways);
			break;
		case QScrollBar::SliderPageStepAdd:
			if(disasmTopLine+int(ceil(visibleLines))-1<int(disasmLines.size()))
				setAddress(disasmLines[disasmTopLine+int(ceil(visibleLines))-1].addr, TopAlways);
			break;
		case QScrollBar::SliderPageStepSub:
			if(disasmTopLine>0)
				setAddress(disasmLines[disasmTopLine-1].addr, BottomAlways);
			break;
		case QScrollBar::SliderToMinimum:
			setAddress(0, TopAlways);
			break;
		case QScrollBar::SliderToMaximum:
			setAddress(0xFFFF, BottomAlways);
			break;
	}
}

// moving the slider is handled separately because
// the SliderMoved action won't catch the extreme values
void DisasmViewer::scrollBarChanged(int value)
{
	setAddress(value, TopAlways);
}


void DisasmViewer::setMemory(unsigned char *memPtr)
{
	memory = memPtr;
	// init disasmLines
	DisasmRow newRow;
	newRow.numBytes = 1;
	newRow.instr = "nop";
	newRow.instr.resize(18, ' ');
	for(int i=0; i<150; i++) {
		newRow.addr = i;
		disasmLines.push_back(newRow);
	}
	disasmTopLine = 50;
}

void DisasmViewer::setBreakpoints(Breakpoints *bps)
{
	breakpoints = bps;
}

void DisasmViewer::keyPressEvent(QKeyEvent *e)
{
	int line;
	
	switch(e->key()) {
		case Qt::Key_Up:
			line = findDisasmLine(cursorAddr);
			if(line>0) 
				cursorAddr = disasmLines[line-1].addr;
			setAddress(cursorAddr, Closest);
			e->accept();
			break;
		case Qt::Key_Down:
			line = findDisasmLine(cursorAddr);
			if(line>=0 && line<int(disasmLines.size())-1) 
				cursorAddr = disasmLines[line+1].addr;
			setAddress(cursorAddr, Closest);
			e->accept();
			break;
		case Qt::Key_PageUp:
			if(disasmTopLine>0)
				setAddress(disasmLines[disasmTopLine-1].addr, BottomAlways);
			e->accept();
			break;
		case Qt::Key_PageDown:
			if(disasmTopLine+int(ceil(visibleLines))-1<int(disasmLines.size()) )
				setAddress(disasmLines[disasmTopLine+int(ceil(visibleLines))-1].addr, TopAlways);
			e->accept();
			break;
		default:
			QFrame::keyReleaseEvent(e);
	}
}

void DisasmViewer::mousePressEvent(QMouseEvent *e)
{
	if(!(e->button()==Qt::LeftButton)) {
		QFrame::mousePressEvent(e);
		return;
	}
	
	if(e->x() >= frameL && e->x() < width()-frameR &&
		e->y() >= frameT && e->y() < height()-frameB)
	{
		// calc clicked line number
		int h = fontMetrics().height();
		int line = (e->y()-frameT)/h;
		
		if(e->x() > frameL+32) {
			// check if the line exists (bottom of memory could have an empty line)
			if(line + disasmTopLine < int(disasmLines.size()))
				cursorAddr = disasmLines[disasmTopLine+line].addr;
			else
				return;
		
			// scroll if partial line
			if(line==int(visibleLines))
				setAddress(disasmLines[disasmTopLine+1].addr, TopAlways);
			else
				update();
		} else if(e->x() < frameL+16) {
			// clicked on the breakpoint area
			if(line + disasmTopLine < int(disasmLines.size()))
				emit toggleBreakpoint(disasmLines[line + disasmTopLine].addr);
		}
	} 
}
