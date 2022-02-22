#ifndef DISASMVIEWER_H
#define DISASMVIEWER_H

#include "Dasm.h"
#include <QFrame>
#include <QPixmap>

class CommMemoryRequest;
class QScrollBar;
class Breakpoints;
class SymbolTable;
struct MemoryLayout;

class DisasmViewer : public QFrame
{
	Q_OBJECT;
public:
	DisasmViewer(QWidget* parent = nullptr);

	void setMemory(unsigned char* memPtr);
	void setBreakpoints(Breakpoints* bps);
	void setMemoryLayout(MemoryLayout* ml);
	void setSymbolTable(SymbolTable* st);
	void memoryUpdated(CommMemoryRequest* req);
	void updateCancelled(CommMemoryRequest* req);
	quint16 programCounter() const;
	quint16 cursorAddress() const;

	QSize sizeHint() const override;

	enum {Top, Middle, Bottom, Closest, TopAlways, MiddleAlways, BottomAlways};

public slots:
	void setAddress(quint16 addr, int infoLine = FIRST_INFO_LINE, int method = Top);
	void setCursorAddress(quint16 addr, int infoLine = FIRST_INFO_LINE, int method = Top);
	void setProgramCounter(quint16 pc);
	void scrollBarAction(int action);
	void scrollBarChanged(int value);
	void updateLayout();
	void refresh();

private:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void wheelEvent(QWheelEvent* e) override;

	QScrollBar* scrollBar;

	QPixmap breakMarker;
	QPixmap watchMarker;
	QPixmap pcMarker;

	quint16 programAddr;
	quint16 cursorAddr;
	int cursorLine;

	QList<int> jumpStack;

	int wheelRemainder;

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
	MemoryLayout* memLayout;
	SymbolTable* symTable;

	int findDisasmLine(quint16 lineAddr, int infoLine = 0);
	int lineAtPos(const QPoint& pos);

signals:
	void toggleBreakpoint(int addr);
};

#endif // DISASMVIEWER_H
