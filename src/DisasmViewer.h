// $Id$

#ifndef _DISASMVIEWER_H
#define _DISASMVIEWER_H

#include <QFrame>
#include <QScrollBar>
#include <QPixmap>

#include "CommClient.h"
#include "Dasm.h"

#include "DebuggerData.h"

class CommMemoryRequest : public CommDebuggableRequest
{
public:
	CommMemoryRequest(int readAt = 0, 
	                  int length = 0, 
	                  unsigned char *targetPtr = NULL, 
	                  int writeAt = 0)
		: CommDebuggableRequest(DISASM_MEMORY_REQ_ID, "memory", readAt, length, targetPtr, writeAt) {};

	int address;
	int method;
	bool repaint;
};



class DisasmViewer : public QFrame
{
	Q_OBJECT;
public:
	DisasmViewer( QWidget* parent = 0 );

	void setMemory(unsigned char *memPtr);
	void setBreakpoints(Breakpoints *bps);
	void memoryUpdated(CommMemoryRequest *req);
	void updateCancelled(CommMemoryRequest *req);

	quint16 programCounter;
	quint16 cursorAddr;

public slots:
	void setAddress(quint16 addr, int method = Top, bool doRepaint = TRUE);
	void setProgramCounter(quint16 pc);
    void scrollBarAction(int action);
	void scrollBarChanged(int value);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void mousePressEvent(QMouseEvent *e);

private:
	enum {Top, Middle, Bottom, Closest, TopAlways, MiddleAlways, BottomAlways};

	QScrollBar *scrollBar;

	QPixmap breakMarker;
	QPixmap pcMarker;

	int frameL, frameR, frameT, frameB;
	double visibleLines;
    int disasmTopLine;
	DisasmLines disasmLines;
	unsigned char *memory;
	int waitingForData;
	CommMemoryRequest *nextRequest;
	Breakpoints *breakpoints;
	
	int findDisasmLine(quint16 lineAddr);
	
signals:
	void needUpdate(CommDebuggableRequest *r);
};

#endif    // _DISASMVIEWER_H
