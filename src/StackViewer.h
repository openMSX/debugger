// $Id$

#ifndef STACKVIEWER_H
#define STACKVIEWER_H

#include <QFrame>

class StackRequest;
class QScrollBar;
class QPaintEvent;

class StackViewer : public QFrame
{
	Q_OBJECT;
public:
	StackViewer(QWidget* parent = 0);

	void setData(unsigned char* memPtr, int memLength);
	void memdataTransfered(StackRequest* r);
	void transferCancelled(StackRequest* r);

public slots:
	void setLocation(int addr);
	void setStackPointer(quint16 addr);

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

private:
	QScrollBar* vertScrollBar;

	int frameL, frameR, frameT, frameB;
	double visibleLines;
	bool waitingForData;

	int stackPointer;
	int topAddress;
	unsigned char* memory;
	int memoryLength;

	void setSizes();
	void setScrollBarValues();
};

#endif // STACKVIEWER_H
