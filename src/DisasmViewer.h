// $Id$

#ifndef DISASMVIEWER_H
#define DISASMVIEWER_H

#include "Dasm.h"
#include <QFrame>
#include <QPixmap>

class CommMemoryRequest;
class QScrollBar;
class Breakpoints;
struct MemoryLayout;
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
	quint16 programCounter() const;
	quint16 cursorAddress() const;

	QSize sizeHint() const;

public slots:
	void setAddress(quint16 addr, int infoLine = FIRST_INFO_LINE, int method = Top);
	void setProgramCounter(quint16 pc);
	void scrollBarAction(int action);
	void scrollBarChanged(int value);
	void settingsChanged();
	void symbolsChanged();
	
protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);
	void keyPressEvent(QKeyEvent* e);
	void mousePressEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);

private:
	enum {Top, Middle, Bottom, Closest, TopAlways, MiddleAlways, BottomAlways};

	QScrollBar* scrollBar;

	QPixmap breakMarker;
	QPixmap pcMarker;

	quint16 programAddr;
	quint16 cursorAddr;
	int	cursorLine;
	
	// layout information
	int frameL, frameR, frameT, frameB;
	int labelFontHeight, labelFontAscent;
	int codeFontHeight,  codeFontAscent;
	int xAddr, xMCode[4], xMnem, xMnemArg;
	int visibleLines, partialBottomLine;
	int disasmTopLine;
	DisasmLines disasmLines;
	
	// display data
	unsigned char* memory;
	bool waitingForData;
	CommMemoryRequest* nextRequest;
	Breakpoints* breakpoints;
	MemoryLayout *memLayout;
	SymbolTable *symTable;

	int findDisasmLine(quint16 lineAddr, int infoLine = 0);
	int lineAtPos( const QPoint& pos );
	
signals:
	void toggleBreakpoint(int addr);
};

#endif // DISASMVIEWER_H
