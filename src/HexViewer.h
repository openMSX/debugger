// $Id$

#ifndef _HEXVIEWER_H
#define _HEXVIEWER_H

#include <QFrame>
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>

#include "CommClient.h"


class HexViewer : public QFrame
{
	Q_OBJECT;
public:
	HexViewer( QWidget* parent = 0 );

	void setData(const char *name, unsigned char *datPtr, int datLength);
	void hexdataTransfered(CommDebuggableRequest *r);
	void transferCancelled(CommDebuggableRequest *r);
	void refresh();

public slots:
	void setLocation(int addr);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

private:
//	QScrollBar *horScrollBar;
	QScrollBar *vertScrollBar;

	int frameL, frameR, frameT, frameB;
	short horBytes;
	double visibleLines;
	bool waitingForData;

	QString dataName;
//	int cursorAddr;
	int hexTopAddress;
	unsigned char *hexData;
	int hexDataLength;

	void setScrollBarValues();

signals:
	void needUpdate(CommDebuggableRequest *r);
};

#endif    // _HEXVIEWER_H
