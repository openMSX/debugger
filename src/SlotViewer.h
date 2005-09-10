// $Id$

#ifndef _SLOTVIEWER_H
#define _SLOTVIEWER_H

#include <QFrame>

#include "CommClient.h"

struct Page {
	char ps;
	char ss;
	quint8 segment;
};

class SlotViewer : public QFrame
{
	Q_OBJECT;
public:
	SlotViewer( QWidget* parent = 0 );

	void refresh();
	void slotsUpdated(CommCommandRequest *r);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

private:
	int frameL, frameR, frameT, frameB;
	int headerSize1, headerSize2, headerSize3, headerSize4;
	int headerHeight;

	Page pages[4];
	bool slotsChanged[4];
	bool segmentsChanged[4];

	void setSizes();

signals:
	void needUpdate(CommCommandRequest *r);
};

#endif    // _SLOTVIEWER_H
