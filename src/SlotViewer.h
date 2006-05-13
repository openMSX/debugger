// $Id$

#ifndef _SLOTVIEWER_H
#define _SLOTVIEWER_H

#include <QFrame>

class MemoryLayout;

class SlotViewer : public QFrame
{
	Q_OBJECT;
public:
	SlotViewer( QWidget* parent = 0 );

	void refresh();
	void setMemoryLayout(MemoryLayout *ml);
	void slotsUpdated(const QString& message);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

private:
	int frameL, frameR, frameT, frameB;
	int headerSize1, headerSize2, headerSize3, headerSize4;
	int headerHeight;

	MemoryLayout *memLayout;

	bool slotsChanged[4];
	bool segmentsChanged[4];

	void setSizes();
};

#endif    // _SLOTVIEWER_H
