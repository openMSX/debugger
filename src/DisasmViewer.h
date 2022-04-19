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
	Q_OBJECT
public:
	DisasmViewer(QWidget* parent = nullptr);

	void setMemory(unsigned char* memPtr);
	void setBreakpoints(Breakpoints* bps);
	void setMemoryLayout(MemoryLayout* ml);
	void setSymbolTable(SymbolTable* st);
	void memoryUpdated(CommMemoryRequest* req);
	void updateCancelled(CommMemoryRequest* req);
	uint16_t programCounter() const;
	uint16_t cursorAddress() const;

	QSize sizeHint() const override;

	enum {Top, Middle, Bottom, Closest, TopAlways, MiddleAlways, BottomAlways, Reload};

public slots:
	void setAddress(uint16_t addr, int infoLine = FIRST_INFO_LINE, int method = Top);
	void setCursorAddress(uint16_t addr, int infoLine = FIRST_INFO_LINE, int method = Top);
	void setProgramCounter(uint16_t pc, bool reload = false);
	void scrollBarAction(int action);
	void scrollBarChanged(int value);
	void updateLayout();
	void refresh();

private:
	void requestMemory(uint16_t start, uint16_t end, uint16_t addr, int infoLine, int method);
	int findPosition(uint16_t addr, int infoLine, int method);

	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void wheelEvent(QWheelEvent* e) override;

	QScrollBar* scrollBar;

	QPixmap breakMarker;
	QPixmap watchMarker;
	QPixmap pcMarker;

	uint16_t programAddr;
	uint16_t cursorAddr;
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

	int findDisasmLine(uint16_t lineAddr, int infoLine = 0);
	int lineAtPos(const QPoint& pos);

signals:
    void breakpointToggled(uint16_t addr);
};

#endif // DISASMVIEWER_H
