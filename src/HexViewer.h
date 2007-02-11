// $Id$

#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QFrame>

class HexRequest;
class QScrollBar;
class QPaintEvent;

class HexViewer : public QFrame
{
	Q_OBJECT
public:
	HexViewer(QWidget* parent = 0);

	void setDebuggable( const QString& name, int size );
	void refresh();

	QSize sizeHint() const;

public slots:
	void setLocation(int addr);
	void settingsChanged();
	
protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

private:
	void setSizes();
	void hexdataTransfered(HexRequest* r);
	void transferCancelled(HexRequest* r);

	QScrollBar* vertScrollBar;

	// layout
	int frameL, frameR, frameT, frameB;
	bool adjustToWidth;
	short horBytes;
	int visibleLines, partialBottomLine;
	int	lineHeight, xAddr, xData, dataWidth, charWidth;
	int addressLength;
	
	// data
	QString debuggableName;
	int debuggableSize;
	int hexTopAddress;
	unsigned char* hexData;
	bool waitingForData;

	friend class HexRequest;
};

#endif // HEXVIEWER_H
