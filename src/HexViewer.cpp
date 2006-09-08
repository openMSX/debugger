// $Id$

#include "HexViewer.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>


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

	// TODO public data is ugly!!
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

	setFont(QFont("Courier New", 12));

	horBytes = 16;
	hexTopAddress = 0;
	waitingForData = false;
	
	vertScrollBar = new QScrollBar(Qt::Vertical, this);
	vertScrollBar->hide();
	
	frameL = frameT = frameB = frameWidth();
	frameR = frameL + vertScrollBar->sizeHint().width();

	connect(vertScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setLocation(int)));
}

void HexViewer::setScrollBarValues()
{
	vertScrollBar->setMinimum(0);

	visibleLines = double(height() - frameT - frameB) / fontMetrics().height();
	
	int maxLine = int(ceil(double(hexDataLength) / horBytes)) - int(visibleLines);
	if (maxLine < 0) maxLine = 0;
	vertScrollBar->setMaximum(maxLine);
	vertScrollBar->setSingleStep(1);
	vertScrollBar->setPageStep(int(visibleLines));
}

void HexViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);

	setScrollBarValues();
	vertScrollBar->setGeometry(width() - frameR, frameT,
	                           vertScrollBar->sizeHint().width(),
	                           height() - frameT - frameB);
	vertScrollBar->show();
	// calc the number of lines that can be displayed
	// partial lines count as a whole
}

void HexViewer::paintEvent(QPaintEvent* e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);

	QPainter p(this);
	int h = fontMetrics().height();
	int d = fontMetrics().descent();

	// set font
	p.setPen(Qt::black);

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > width()  - frameR - 1) r.setRight (width()  - frameR - 1);
	if (r.bottom() > height() - frameB - 1) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect( r, palette().color(QPalette::Base) );

	// calc layout (not optimal)
	int charWidth = fontMetrics().width("A");
	int spacing = charWidth / 2;
	int xAddr = frameL + 8;
	int xHex1 = xAddr + 6 * charWidth;
	int dHex = 2 * charWidth + spacing;

	int y = frameT + h - 1;
	
	int address = hexTopAddress;

	for (int i = 0; i < int(ceil(visibleLines)); ++i) {
		// print address
		QString hexStr;
		hexStr.sprintf("%04X", address);
		p.drawText(xAddr, y - d, hexStr);
		// print bytes
		int x = xHex1;
		for (int addr = address; addr < address + horBytes; ++addr) {
			// at extra spacing halfway
			if (!(horBytes & 1)) {
				if (addr - address == horBytes / 2) {
					x += spacing;
				}
			}
			// print data (if there still is any)
			if (addr < hexDataLength) {
				hexStr.sprintf("%02X", hexData[addr]);
				p.drawText(x, y - d, hexStr);
			}
			x += dHex;
		}
		x += 2 * spacing;
		hexStr.clear();
		for (int addr = address; addr < address + horBytes; ++addr) {
			if (addr >= hexDataLength) break;
			unsigned char chr = hexData[addr];
			if (chr < 32 || chr > 127) chr = '.';
			hexStr += chr;
		}
		p.drawText(x, y - d, hexStr);
		y += h;
		address += horBytes;
		if (address >= hexDataLength) break;
	}
}

void HexViewer::setData(const char* name, unsigned char* datPtr, int datLength)
{
	dataName = name;
	hexData = datPtr;
	hexDataLength = datLength;
	setScrollBarValues();
}

void HexViewer::setLocation(int addr)
{	
	if (!waitingForData) {
		int start = addr * horBytes;
		int size = horBytes * int(ceil(visibleLines));

		if (start + size > hexDataLength) {
			size = hexDataLength - start;
		}
		HexRequest* req = new HexRequest(
			dataName, start, size, &hexData[start], *this);
		CommClient::instance().sendCommand(req);
		waitingForData = TRUE;
	}
}

void HexViewer::hexdataTransfered(HexRequest* r)
{
	hexTopAddress = r->offset;
	update();
	transferCancelled(r);
}

void HexViewer::transferCancelled(HexRequest* r)
{
	delete r;
	waitingForData = false;
	// check whether a new value is available
	if (hexTopAddress != vertScrollBar->value() * horBytes) {
		setLocation(vertScrollBar->value());
	}
}

void HexViewer::refresh()
{
	setLocation(vertScrollBar->value());
}
