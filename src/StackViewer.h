// $Id$

#ifndef _STACKVIEWER_H
#define _STACKVIEWER_H

#include <QFrame>
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>

#include "CommClient.h"


class StackViewer : public QFrame
{
	Q_OBJECT;
public:
	StackViewer( QWidget* parent = 0 );

	void setData(unsigned char *memPtr, int memLength);
	void memdataTransfered(CommDebuggableRequest *r);
	void transferCancelled(CommDebuggableRequest *r);

public slots:
	void setLocation(int addr);
	void setStackPointer(quint16 addr);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

private:
	QScrollBar *vertScrollBar;

	int frameL, frameR, frameT, frameB;
	double visibleLines;
	bool waitingForData;

	int stackPointer;
    int topAddress;
	unsigned char *memory;
	int memoryLength;

	void setSizes();
	void setScrollBarValues();

signals:
	void needUpdate(CommDebuggableRequest *r);
};

#endif    // _STACKVIEWER_H
