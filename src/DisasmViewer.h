// $Id$

#ifndef DISASMVIEWER_H
#define DISASMVIEWER_H

#include "Dasm.h"
#include <QFrame>
#include <QPixmap>

class CommMemoryRequest;
class QScrollBar;
class Breakpoints;
class MemoryLayout;
class SymbolTable;

class DisasmViewer : public QFrame
{
	Q_OBJECT;
public:
	DisasmViewer(QWidget* parent = 0);

	void setMemory(unsigned char* memPtr);
	void setBreakpoints(Breakpoints* bps);
	void setMemoryLayout(MemoryLayout* ml);
	void setSymbolTable(SymbolTable* st);
	void memoryUpdated(CommMemoryRequest* req);
	void updateCancelled(CommMemoryRequest* req);

	// TODO get rid of public members
	quint16 programCounter;
	quint16 cursorAddr;

public slots:
	void setAddress(quint16 addr, int method = Top);
	void setProgramCounter(quint16 pc);
	void scrollBarAction(int action);
	void scrollBarChanged(int value);

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);
	void keyPressEvent(QKeyEvent* e);
	void mousePressEvent(QMouseEvent* e);

private:
	enum {Top, Middle, Bottom, Closest, TopAlways, MiddleAlways, BottomAlways};

	QScrollBar* scrollBar;

	QPixmap breakMarker;
	QPixmap pcMarker;

	int frameL, frameR, frameT, frameB;
	double visibleLines;
	int disasmTopLine;
	DisasmLines disasmLines;
	unsigned char* memory;
	int waitingForData;
	CommMemoryRequest* nextRequest;
	Breakpoints* breakpoints;
	MemoryLayout *memLayout;
	SymbolTable *symTable;

	int findDisasmLine(quint16 lineAddr);

signals:
	void toggleBreakpoint(int addr);
};

#endif // DISASMVIEWER_H
