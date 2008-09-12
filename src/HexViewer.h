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
	~HexViewer();
	
	void setDebuggable( const QString& name, int size );
	void setEnabledScrollBar( bool enabled );
	void setUseMarker( bool enabled );
	void setIsEditable( bool enabled );

	QSize sizeHint() const;

public slots:
	void setLocation(int addr);
	void setTopLocation(int addr);
	void scrollBarChanged(int addr);
	void settingsChanged();
	void refresh();

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);
	bool event(QEvent *e);
	void keyPressEvent(QKeyEvent *e);

private:
	void setSizes();
	void hexdataTransfered(HexRequest* r);
	void transferCancelled(HexRequest* r);

	QScrollBar* vertScrollBar;

	// layout
	int frameL, frameR, frameT, frameB;
	int leftCharPos, leftValuePos, rightValuePos, rightCharPos ;
	bool adjustToWidth;
	short horBytes;
	int visibleLines, partialBottomLine;
	int	lineHeight, xAddr, xData, xChar, dataWidth, charWidth;
	int addressLength;
	
	// data
	QString debuggableName;
	int debuggableSize;
	int hexTopAddress;
	int hexMarkAddress;
	unsigned char* hexData;
	unsigned char* previousHexData;
	bool waitingForData;
	bool highlitChanges;
	bool useMarker;
	bool isEditable;
	bool beingEdited;
	int cursorPosition,editValue;

	friend class HexRequest;

signals:
	void locationChanged(int addr);
};

#endif // HEXVIEWER_H
