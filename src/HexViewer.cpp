// $Id$

#include "HexViewer.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>

static const int EXTRA_SPACING = 4;

class HexRequest : public ReadDebugBlockCommand
{
public:
	HexRequest(const QString& debuggable, unsigned offset_, unsigned size,
	           unsigned char* target, HexViewer& viewer_)
		: ReadDebugBlockCommand(debuggable, offset_, size, target)
		, offset(offset_)
		, viewer(viewer_)
	{
	}

	virtual void replyOk(const QString& message)
	{
		copyData(message);
		viewer.hexdataTransfered(this);
	}

	virtual void cancel()
	{
		viewer.transferCancelled(this);
	}

	unsigned offset;

private:
	HexViewer& viewer;
};


HexViewer::HexViewer(QWidget* parent)
	: QFrame(parent)
{
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding ) );
	
	horBytes = 16;
	hexTopAddress = 0;
	hexMarkAddress = 0;
	hexData = NULL;
	previousHexData = NULL;
	debuggableSize = 0;
	waitingForData = false;
	adjustToWidth = true;
	highlitChanges = true;
	addressLength = 4;
	
	vertScrollBar = new QScrollBar(Qt::Vertical, this);
	vertScrollBar->setMinimum(0);
	vertScrollBar->setSingleStep(1);

	frameL = frameT = frameB = frameWidth();
	frameR = frameL + vertScrollBar->sizeHint().width();

	settingsChanged();
	
	connect(vertScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarChanged(int)));
}

HexViewer::~HexViewer()
{
	delete[] hexData;
	delete[] previousHexData;
}

void HexViewer::setUseMarker(bool enabled)
{
	useMarker = enabled;
	// below we should check if current marker is visible etc
	// but since in the debugger we will set this at instanciation
	// and then never change it again this is a quicky in case we do later on
	if (useMarker){
		hexMarkAddress = hexTopAddress;
	}
	update();
}

void HexViewer::setEnabledScrollBar(bool enabled)
{
	vertScrollBar->setEnabled( enabled );
}

void HexViewer::settingsChanged()
{
	QFontMetrics fm( Settings::get().font( Settings::HEX_FONT ) );
	
	lineHeight = fm.height();
	charWidth = fm.width("W");
	xAddr = frameL + 8;
	xData = xAddr + addressLength*charWidth + charWidth;
	dataWidth = 5*charWidth/2;
	setSizes();
}

void HexViewer::setSizes()
{
	visibleLines = (height() - frameT - frameB) / lineHeight;
	partialBottomLine = (height() - frameT - frameB) != lineHeight*visibleLines;
	
	frameR = frameL;
	int w;
	// fit display to width
	if( adjustToWidth ) {
		horBytes = 1;
		w = width() - frameL - frameR - xData - dataWidth - 2*charWidth - 8;
		// calculate how many additional bytes can by displayed
		while( w >= dataWidth+charWidth ) {
			horBytes++;
			w -= dataWidth+charWidth;
			if( (horBytes&3) == 0 ) w -= EXTRA_SPACING;
			if( (horBytes&7) == 0 ) w -= EXTRA_SPACING;
		}
	}
	
	// check if a scrollbar is needed
	if( horBytes*visibleLines < debuggableSize ) {
		if( adjustToWidth && w < vertScrollBar->sizeHint().width() ) horBytes--;
		int maxLine = int(ceil(double(debuggableSize)/horBytes)) - visibleLines;
		if( maxLine < 0 ) maxLine = 0;
		vertScrollBar->setMaximum(maxLine);
		vertScrollBar->setPageStep(visibleLines);
		frameR += vertScrollBar->sizeHint().width();
		vertScrollBar->setGeometry(width() - frameR, frameT,
		                           vertScrollBar->sizeHint().width(),
	    	                       height() - frameT - frameB);
		vertScrollBar->show();
	} else {
		vertScrollBar->hide();
		hexTopAddress = 0;
		hexMarkAddress = 0;
	}

	if( isEnabled() ) {
		///vertScrollBar->setValue(hexTopAddress/horBytes);
		setTopLocation(horBytes*int(hexTopAddress/horBytes));
	} else {
		update();
	}
}

QSize HexViewer::sizeHint() const
{
	return QSize( frameL + 16 + (6+3*horBytes/2)*fontMetrics().width("A") + frameR,
	              frameT + 10*fontMetrics().height() + frameB );
}

void HexViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);

	setSizes();
}

void HexViewer::paintEvent(QPaintEvent* e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);

	// exit if no debuggable is set
	if( debuggableName.isEmpty() ) return;

	QPainter p(this);
	
	// set font info
	p.setFont( Settings::get().font( Settings::HEX_FONT ) );
	QColor fc( Settings::get().fontColor( Settings::HEX_FONT ) );
	int a = p.fontMetrics().ascent();

	p.setPen( fc );

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > width()  - frameR - 1) r.setRight (width()  - frameR - 1);
	if (r.bottom() > height() - frameB - 1) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect( r, palette().color(QPalette::Base) );

	int y = frameT;
	
	int address = hexTopAddress;

	for (int i = 0; i < visibleLines+partialBottomLine; ++i) {
		// print address
		QString hexStr = QString("%1").arg(address, addressLength, 16, QChar('0'));
		p.setPen( palette().color(QPalette::Text) );
		p.drawText(xAddr, y + a, hexStr.toUpper());

		// print bytes
		int x = xData;
		for( int j = 0; j < horBytes; j++ ) {
			// print data
			if (address + j < debuggableSize) {
				hexStr.sprintf("%02X", hexData[address + j]);
				// draw marker if needed
				if (useMarker && ((address + j) == hexMarkAddress)){
					QRect b(x,y,dataWidth,lineHeight);
					p.fillRect( b, Qt::lightGray);
				};
				// determine value colour
				if (highlitChanges) {
					QColor penClr = palette().color(QPalette::Text);
					if (hexData[address + j] != previousHexData[address + j]){
						penClr = Qt::red;
					}
					p.setPen( penClr );
				};
				p.drawText(x, y + a, hexStr);
			}
			x += dataWidth;
			// at extra spacing
			if( (j&3) == 3 ) x += EXTRA_SPACING;
			if( (j&7) == 7 ) x += EXTRA_SPACING;
		}

		// print characters
		x += charWidth;
		for( int j = 0; j < horBytes; j++ ) {
			if (address + j >= debuggableSize) break;
			unsigned char chr = hexData[address + j];
			if (chr < 32 || chr > 127) chr = '.';
			// draw marker if needed
			if (useMarker && ((address + j) == hexMarkAddress)){
				QRect b(x,y,charWidth,lineHeight);
				p.fillRect( b, Qt::lightGray);
			};
			// determine value colour
			if (highlitChanges) {
				QColor penClr = palette().color(QPalette::Text);
				if (hexData[address + j] != previousHexData[address + j]){
					penClr = Qt::red;
				}
				p.setPen( penClr );
			}
			p.drawText(x, y + a, QString(chr));
			x += charWidth;
		}
		
		y += lineHeight;
		address += horBytes;
		if (address >= debuggableSize) break;
	}
	// copy the new values to the old-values buffer
	memcpy( previousHexData, hexData, debuggableSize );
}

void HexViewer::setDebuggable( const QString& name, int size )
{
	delete[] hexData;
	hexData = NULL;
	delete[] previousHexData;
	previousHexData = NULL;

	if( size ) {
		debuggableName = name;
		debuggableSize = size;
		addressLength = 2 * int(ceil( log(size) / log(2) / 8 ));
		hexTopAddress = 0;
		hexMarkAddress = 0;
		hexData = new unsigned char[size];
		memset( hexData, 0, size );
		previousHexData = new unsigned char[size];
		memset( previousHexData, 0, size );
		settingsChanged();
	} else {
		debuggableName.clear();
		debuggableSize = 0;
	}
}

void HexViewer::scrollBarChanged(int addr)
{
	int start = addr * horBytes;
	if (start == hexTopAddress){
		// nothing changed or a callback since we changed the value to
		// the current hexTopAddress
		return;
	};

	if (!useMarker){
		setTopLocation(addr * horBytes);
		emit locationChanged(addr * horBytes);
	} else {
		//maybe marker is still visible ?
		int size = horBytes * (visibleLines+partialBottomLine);
		hexTopAddress = start;
		if ((start > hexMarkAddress) || ((start + size -1) < hexMarkAddress) ){
			hexMarkAddress = (start<hexMarkAddress)?(size-1):0;
			hexMarkAddress += start;
			emit locationChanged(hexMarkAddress);
		}
		refresh();
	}
}

void HexViewer::setLocation(int addr)
{
	if (!useMarker){
		setTopLocation(addr);
	} else {
		//check if newmarker is in first 1/3th of hexviewer if so we do not change hexTopAddress
		hexMarkAddress = addr;
		int size = horBytes * int ((visibleLines+partialBottomLine)/3);
		if ((addr < hexTopAddress) || (addr>(hexTopAddress+size))){
			setTopLocation(addr);
		}
		refresh();
	}
}

void HexViewer::setTopLocation(int addr)
{

	if (debuggableName.isEmpty()) {
		return;
	};
	int start = horBytes * int(addr/horBytes);
	if ((!waitingForData) || (start != hexTopAddress )) {
		hexTopAddress = start;
		refresh();
	}
}

void HexViewer::hexdataTransfered(HexRequest* r)
{
	transferCancelled(r);
	update();
}

void HexViewer::transferCancelled(HexRequest* r)
{
	delete r;
	waitingForData = false;
	// check whether a new value is available
	if (int(hexTopAddress / horBytes) != vertScrollBar->value() ) {
		vertScrollBar->setValue(hexTopAddress / horBytes);
	}
}

void HexViewer::refresh()
{
	// calculate data request
	int size = horBytes * (visibleLines+partialBottomLine);

	if (hexTopAddress + size > debuggableSize) {
		size = debuggableSize - hexTopAddress;
	}

	// send data request
	HexRequest* req = new HexRequest(
			debuggableName, hexTopAddress, size, hexData + hexTopAddress, *this);
	CommClient::instance().sendCommand(req);
	waitingForData = true;
}
