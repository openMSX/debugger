// $Id$

#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QFrame>

class HexRequest;
class QScrollBar;
class QPaintEvent;

class HexViewer : public QFrame
{
	Q_OBJECT;
public:
	HexViewer(QWidget* parent = 0);

	void setData(const char* name, unsigned char* datPtr, int datLength);
	void refresh();

	QSize sizeHint() const;

public slots:
	void setLocation(int addr);

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

private:
	void setScrollBarValues();
	void hexdataTransfered(HexRequest* r);
	void transferCancelled(HexRequest* r);

	QScrollBar* vertScrollBar;

	int frameL, frameR, frameT, frameB;
	short horBytes;
	double visibleLines;
	bool waitingForData;

	QString dataName;
	int hexTopAddress;
	unsigned char* hexData;
	int hexDataLength;

	friend class HexRequest;
};

#endif // HEXVIEWER_H
