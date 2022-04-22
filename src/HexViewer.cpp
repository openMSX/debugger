#include "HexViewer.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>
#include <QAction>
#include <algorithm>
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

	void replyOk(const QString& message) override
	{
		copyData(message);
		viewer.hexdataTransfered(this);
	}

	void cancel() override
	{
		viewer.transferCancelled(this);
	}

	unsigned offset;

private:
	HexViewer& viewer;
};


HexViewer::HexViewer(QWidget* parent)
	: QFrame(parent)
    , wheelRemainder(0),horBytes{16}
{
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

	hexTopAddress = 0;
	hexMarkAddress = 0;
	hexData = nullptr;
	previousHexData = nullptr;
	debuggableSize = 0;
	waitingForData = false;
	displayMode = FILL_WIDTH;
	highlitChanges = true;
	addressLength = 4;
	isEditable = false;
	isInteractive = false;
	beingEdited = false;
	editedChars = false;
	useMarker = false;
	hasFocus = false;

	vertScrollBar = new QScrollBar(Qt::Vertical, this);
	vertScrollBar->setMinimum(0);
	vertScrollBar->setSingleStep(1);

	frameL = frameT = frameB = frameWidth();
	frameR = frameL + vertScrollBar->sizeHint().width();

	settingsChanged();

	createActions();

	connect(vertScrollBar, SIGNAL(valueChanged(int)),
	        this, SLOT(scrollBarChanged(int)));
}

HexViewer::~HexViewer()
{
	delete[] hexData;
	delete[] previousHexData;
}

void HexViewer::createActions()
{
	fillWidthAction = new QAction(tr("&Fill with"), this);
	fillWidthAction->setShortcut(tr("Ctrl+F"));
	fillWidthAction->setStatusTip(tr("Fill the width with as many bytes as possible."));

	fillWidth2Action = new QAction(tr("Power of &2"), this);
	fillWidth2Action->setShortcut(tr("Ctrl+2"));
	fillWidth2Action->setStatusTip(tr("Fill the with with the maximum power of 2 possible."));

	setWith8Action = new QAction(tr("&8 bytes."), this);
	setWith8Action->setShortcut(tr("Ctrl+8"));
	setWith8Action->setStatusTip(tr("Set width to 8 bytes."));

	setWith16Action = new QAction(tr("&16 bytes."), this);
	setWith16Action->setShortcut(tr("Ctrl+6"));
	setWith16Action->setStatusTip(tr("Set width to 16 bytes."));

	setWith32Action = new QAction(tr("&32 bytes."), this);
	setWith32Action->setShortcut(tr("Ctrl+3"));
	setWith32Action->setStatusTip(tr("Set width to 32 bytes."));

	connect(fillWidthAction,  SIGNAL(triggered()), this, SLOT(changeWidth()));
	connect(fillWidth2Action, SIGNAL(triggered()), this, SLOT(changeWidth()));
	connect(setWith8Action,   SIGNAL(triggered()), this, SLOT(changeWidth()));
	connect(setWith16Action,  SIGNAL(triggered()), this, SLOT(changeWidth()));
	connect(setWith32Action,  SIGNAL(triggered()), this, SLOT(changeWidth()));

	addAction(fillWidthAction);
	addAction(fillWidth2Action);
	addAction(setWith8Action);
	addAction(setWith16Action);
	addAction(setWith32Action);
	setContextMenuPolicy(Qt::ActionsContextMenu);
}

void HexViewer::changeWidth()
{
	if (sender() == fillWidthAction) setDisplayMode(FILL_WIDTH);
	else if (sender() == fillWidth2Action) setDisplayMode(FILL_WIDTH_POWEROF2);
	else if (sender() == setWith8Action) setDisplayWidth(8);
	else if (sender() == setWith16Action) setDisplayWidth(16);
	else if (sender() == setWith32Action) setDisplayWidth(32);
}

void HexViewer::setIsEditable(bool enabled)
{
	isEditable = enabled;
	setUseMarker(true);
}

void HexViewer::setUseMarker(bool enabled)
{
	useMarker = enabled;
	// below we should check if current marker is visible etc
	// but since in the debugger we will set this at instantiation
	// and then never change it again this is a quicky in case we do later on
	if (useMarker) {
		hexMarkAddress = hexTopAddress;
	}
	update();
}

void HexViewer::setIsInteractive(bool enabled)
{
	isInteractive = enabled;
	vertScrollBar->setEnabled(enabled);
}

void HexViewer::setDisplayMode(Mode mode)
{
	displayMode = mode;
	setSizes();
}

void HexViewer::setDisplayWidth(short width)
{
	displayMode = FIXED;
    horBytes = std::max(short{1},width);
	setSizes();
}

void HexViewer::settingsChanged()
{
	QFontMetrics fm(Settings::get().font(Settings::HEX_FONT));
	lineHeight = fm.height();
	charWidth = fm.horizontalAdvance("W");
	hexCharWidth = fm.horizontalAdvance("0ABCDEF") / 7;
	xAddr = frameL + 8;
	xData = xAddr + addressLength * hexCharWidth + charWidth;
	dataWidth = 3 * hexCharWidth;
	setSizes();
}

void HexViewer::setSizes()
{
	visibleLines = (height() - frameT - frameB) / lineHeight;
	partialBottomLine = (height() - frameT - frameB) != lineHeight * visibleLines;

	frameR = frameL;
	int w;

	// fit display to width
	if (displayMode != FIXED) {
		// scrollbar width
		int sbw = vertScrollBar->sizeHint().width();
		horBytes = 1;
		int hb2 = 1;
		w = width() - frameL - frameR - xData - dataWidth - 2 * charWidth - 8;
		// calculate how many additional bytes can by displayed
		while (w-sbw >= dataWidth + charWidth) {
			++horBytes;
			if (horBytes == 2 * hb2) hb2 = horBytes;
			w -= dataWidth + charWidth;
			if ((horBytes & 3) == 0) w -= EXTRA_SPACING;
			if ((horBytes & 7) == 0) w -= EXTRA_SPACING;
			// remove scrollbar
			if (horBytes * visibleLines >= debuggableSize) sbw = 0;
		}
		// limit to power of two if needed
		if (displayMode == FILL_WIDTH_POWEROF2)
			horBytes = hb2;
	}

	// check if a scrollbar is needed
	if (horBytes * visibleLines < debuggableSize) {
		int maxLine = int(ceil(double(debuggableSize) / horBytes)) - visibleLines;
		maxLine = std::max(maxLine, 0);
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

	// now see were the chars are drawn
	rightValuePos = xData + horBytes * dataWidth;
	xChar = rightValuePos + charWidth + EXTRA_SPACING * (int(horBytes / 4) +
	        int(horBytes / 8));
	rightCharPos = xChar + horBytes * charWidth;

	if (isEnabled()) {
		///vertScrollBar->setValue(hexTopAddress / horBytes);
		setTopLocation(horBytes * int(hexTopAddress / horBytes));
	} else {
		update();
	}
}

QSize HexViewer::sizeHint() const
{
	return {frameL + 16 + (6 + 3 * horBytes / 2) * fontMetrics().horizontalAdvance("A") + frameR,
	        frameT + 10 * fontMetrics().height() + frameB};
}

void HexViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);
	setSizes();
}

void HexViewer::wheelEvent(QWheelEvent* e)
{
	wheelRemainder += e->angleDelta().y();
	const int delta = wheelRemainder / 40;
	wheelRemainder %= 40;
	if (delta) {
		int v = vertScrollBar->value() - delta;
		vertScrollBar->setValue(v);
	}
	e->accept();
}

void HexViewer::paintEvent(QPaintEvent* e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);

	// exit if no debuggable is set
	if (debuggableName.isEmpty()) return;

	QPainter p(this);

	// set font info
	p.setFont(Settings::get().font(Settings::HEX_FONT));
	QColor fc(Settings::get().fontColor(Settings::HEX_FONT));
	int a = p.fontMetrics().ascent();

	p.setPen(fc);

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > width()  - frameR - 1) r.setRight (width()  - frameR - 1);
	if (r.bottom() > height() - frameB - 1) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect(r, palette().color(QPalette::Base));

	int y = frameT;
	int address = hexTopAddress;
	for (int i = 0; i < visibleLines+partialBottomLine; ++i) {
		// print address
		QString hexStr = QString("%1").arg(address, addressLength, 16, QChar('0'));
		p.setPen(palette().color(QPalette::Text));
		p.drawText(xAddr, y + a, hexStr.toUpper());

		// print bytes
		int x = xData;
		for (int j = 0; j < horBytes; ++j) {
			// print data
			if (address + j < debuggableSize) {
				hexStr = QString("%1").arg(hexData[address + j], 2, 16, QChar('0')).toUpper();
				// draw marker if needed
				if (useMarker || beingEdited) {
					QRect b(x, y, dataWidth, lineHeight);
					if ((address + j) == hexMarkAddress) {
						p.fillRect(b, hasFocus ? Qt::cyan
						                       : Qt::lightGray);
					}
					//  are we being edited ??
					if (hasFocus && isEditable && !editedChars &&
					    ((address + j) == hexMarkAddress)) {
						if (beingEdited) {
							p.fillRect(b, Qt::darkGreen);
							if (cursorPosition) {
								hexStr = QString("%1").arg(editValue, 2, 16).toUpper();
							}
						} else {
							p.drawRect(b);
						}
					}
				}

				// determine value colour
				if (highlitChanges) {
					QColor penClr = palette().color(QPalette::Text);
					if (hexData[address + j] != previousHexData[address + j]) {
						if ((address + j) != hexMarkAddress || !beingEdited) {
							penClr = Qt::red;
						}
					}
					if (((address + j) == hexMarkAddress) &&
					    beingEdited && (cursorPosition == 0)) {
						penClr = Qt::white;
					}
					p.setPen(penClr);
				}
				p.drawText(x, y + a, hexStr);
			}
			x += dataWidth;

			// at extra spacing
			if ((j & 3) == 3) x += EXTRA_SPACING;
			if ((j & 7) == 7) x += EXTRA_SPACING;
		}

		// print characters
		x += charWidth;
		for (int j = 0; j < horBytes; ++j) {
			if (address + j >= debuggableSize) break;
			unsigned char chr = hexData[address + j];
			if (chr < 32 || chr > 127) chr = '.';
			// draw marker if needed
			if (useMarker || beingEdited) {
				QRect b(x, y, charWidth, lineHeight);
				if ((address + j) == hexMarkAddress) {
					p.fillRect(b, hasFocus ? Qt::cyan
					                       : Qt::lightGray);
				}
				// are we being edited ??
				if (hasFocus && isEditable && editedChars &&
				    ((address + j) == hexMarkAddress)) {
					if (beingEdited) {
						p.fillRect(b, Qt::darkGreen);
					} else {
						p.drawRect(b);
					}
				}
			}
			// determine value colour
			if (highlitChanges) {
				QColor penClr = palette().color(QPalette::Text);
				if (hexData[address + j] != previousHexData[address + j]) {
					penClr = Qt::red;
				}
				if (((address + j) == hexMarkAddress) && beingEdited &&
				    (cursorPosition == 0)) {
					penClr = Qt::white;
				}
				p.setPen(penClr);
			}
			p.drawText(x, y + a, QString(chr));
			x += charWidth;
		}

		y += lineHeight;
		address += horBytes;
		if (address >= debuggableSize) break;
	}
	// copy the new values to the old-values buffer
	memcpy(previousHexData, hexData, debuggableSize);
}

void HexViewer::setDebuggable(const QString& name, int size)
{
	delete[] hexData;
	hexData = nullptr;
	delete[] previousHexData;
	previousHexData = nullptr;

	if (size) {
		debuggableName = name;
		debuggableSize = size;
		addressLength = 2 * int(ceil(log(double(size)) / log(2.0) / 8));
		hexTopAddress = 0;
		hexMarkAddress = 0;
		hexData = new unsigned char[size];
		memset(hexData, 0, size);
		previousHexData = new unsigned char[size];
		memset(previousHexData, 0, size);
		settingsChanged();
	} else {
		debuggableName.clear();
		debuggableSize = 0;
	}
}

void HexViewer::scrollBarChanged(int addr)
{
	int start = addr * horBytes;
	if (start == hexTopAddress) {
		// nothing changed or a callback since we changed the value to
		// the current hexTopAddress
		return;
	}

	if (!useMarker) {
		setTopLocation(addr * horBytes);
		emit locationChanged(addr * horBytes);
	} else {
		//maybe marker is still "fully" visible?
		int size = horBytes * visibleLines;
		hexTopAddress = start;
		if ((start > hexMarkAddress) || ((start + size -1) < hexMarkAddress)) {
			hexMarkAddress = (hexMarkAddress % horBytes) +
				         ((start < hexMarkAddress) ? size - horBytes : 0);
			hexMarkAddress += start;
			emit locationChanged(hexMarkAddress);
		}
		refresh();
	}
}

void HexViewer::setLocation(int addr)
{
	if (!useMarker) {
		setTopLocation(addr);
	} else {
		// check if newmarker is in fully visible lines,
		// if so we do not change hexTopAddress
		if (addr != hexMarkAddress) {
			emit locationChanged(addr);
		}
		hexMarkAddress = addr;
		int size = horBytes * visibleLines;
		if ((addr < hexTopAddress) || (addr >= (hexTopAddress+size))) {
			setTopLocation(addr);
		}
		refresh();
	}
}

void HexViewer::setTopLocation(int addr)
{
	if (debuggableName.isEmpty()) {
		return;
	}
	int start = horBytes * int(addr / horBytes);
	if (!waitingForData || (start != hexTopAddress)) {
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
    if (horBytes==0) {
        // for some reasson we could get a floating point exception here when cerating a new Hexviewer in disconnected mode??
        qDebug()<< "Avoided divide by zero??";
        return;
    };
    if (int(hexTopAddress / horBytes) != vertScrollBar->value()) {
		vertScrollBar->setValue(hexTopAddress / horBytes);
	}
}

void HexViewer::refresh()
{
	// calculate data request
	int size = horBytes * (visibleLines + partialBottomLine);
	size = std::min(size, debuggableSize - hexTopAddress);

	// send data request
	auto* req = new HexRequest(
		debuggableName, hexTopAddress, size, hexData + hexTopAddress, *this);
	CommClient::instance().sendCommand(req);
	waitingForData = true;
}

void HexViewer::keyPressEvent(QKeyEvent* e)
{
	// don't hanlde if not interactive
	if ((!beingEdited && !useMarker) || !isInteractive) {
		QFrame::keyPressEvent(e);
		return;
	}

	bool setValue = false;
	int newAddress = hexMarkAddress;
	// entering a new digit ?
	// for now hex only, do we need decimal entry also ??
	if (!editedChars &&
	    ((e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9) ||
	     (e->key() >= Qt::Key_A && e->key() <= Qt::Key_F))) {
		// calculate numercial value
		int v = e->key() - Qt::Key_0;
		if (v > 9) v -= Qt::Key_A - Qt::Key_0 - 10;
		if (beingEdited) {
			editValue = (editValue << 4) + v;
			++cursorPosition;
			if (cursorPosition == 2) {
				setValue = true;
				++newAddress;
			}
		} else {
			editValue = v;
			beingEdited = true;
			cursorPosition = 1;
		}
	} else if (useMarker && e->key() == Qt::Key_Right) {
		setValue = beingEdited & !editedChars;
		++newAddress;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_Left) {
		setValue = beingEdited & !editedChars;
		--newAddress;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_Up) {
		setValue = beingEdited & !editedChars;
		newAddress -= horBytes;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_Down) {
		setValue = beingEdited & !editedChars;
		newAddress += horBytes;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_Home) {
		setValue = beingEdited & !editedChars;
		newAddress = 0;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_PageUp) {
		setValue = beingEdited & !editedChars;
		hexTopAddress -= horBytes * visibleLines;
		newAddress -= horBytes * visibleLines;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_PageDown) {
		setValue = beingEdited & !editedChars;
		hexTopAddress += horBytes * visibleLines;
		newAddress += horBytes * visibleLines;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_End) {
		setValue = beingEdited & !editedChars;
		newAddress = debuggableSize - 1;
		cursorPosition = 0;
	} else if (useMarker && e->key() == Qt::Key_Backspace) {
		editedChars = !editedChars;
	} else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
		if (beingEdited)
			setValue = true;
		else
			cursorPosition = 0;
		if (editedChars)
			editValue = previousHexData[hexMarkAddress];
		newAddress++;
	} else if (e->key() == Qt::Key_Shift    ||
		   e->key() == Qt::Key_Control  ||
		   e->key() == Qt::Key_Meta     ||
		   e->key() == Qt::Key_Alt      ||
		   e->key() == Qt::Key_AltGr    ||
		   e->key() == Qt::Key_CapsLock ||
		   e->key() == Qt::Key_NumLock  ||
		   e->key() == Qt::Key_ScrollLock) {
		// do nothing for these keys if editing chars
		// if not editing chars they do nothing by deafult :-)
	} else if (e->key() == Qt::Key_Escape) {
		beingEdited = false;
		e->accept();
		update();
		return;
	} else if (editedChars) {
		editValue = static_cast<unsigned char>((e->text().toLatin1())[0]);
		setValue = true;
		++newAddress;
	} else {
		QFrame::keyPressEvent(e);
		return;
	}

	//apply changes
	if (setValue) {
		//TODO actually write the values to openMSX memory
		//for now we change the value in our local buffer
		previousHexData[hexMarkAddress] = char(editValue);
		auto* req = new WriteDebugBlockCommand(
			debuggableName, hexMarkAddress, 1, previousHexData);
		CommClient::instance().sendCommand(req);

		editValue = 0;
		cursorPosition = 0;
		beingEdited = editedChars; // keep editing if we were inputing chars
		refresh();
	}

	// indicate key Event handled
	e->accept();

	// Move Marker if needed
	if ((editedChars || useMarker) && (hexMarkAddress != newAddress)) {
		if (newAddress < 0)               newAddress += debuggableSize;
		if (newAddress >= debuggableSize) newAddress -= debuggableSize;
		// influencing hexTopAddress during Key_PageUp/Down might need following 2 lines.
		if (hexTopAddress < 0)               hexTopAddress += debuggableSize;
		if (hexTopAddress >= debuggableSize) hexTopAddress -= debuggableSize;

		// Make scrolling downwards using cursors more "intuitive"
		int addr = hexTopAddress + horBytes * visibleLines;
		if ((newAddress >= addr) && (newAddress <= (addr + horBytes))) {
			hexTopAddress += horBytes;
		}
		if (useMarker) {
			setLocation(newAddress);
		} else {
			//we can only get here when not using Markers but if we
			//are typing in chars in charEdit mode, so scrolling
			//one line is covered in code above
			hexMarkAddress = newAddress;
			refresh();
		}
	} else {
		update();
	}
}

int HexViewer::coorToOffset(int x, int y) const
{
	int offset = -1;
	if (x >= xData && x < rightValuePos) {
		offset = 0;
		x -= xData;
		while (x > 4*dataWidth) {
			offset += 4;
			x -= 4*dataWidth + EXTRA_SPACING;
			if (offset % 8 == 0) x -= EXTRA_SPACING;
		}
		offset += x / dataWidth;
	} else if (x >= xChar && x < rightCharPos) {
		offset = (x - xChar) / charWidth;
	}
	int yMaxOffset = frameT + (visibleLines+partialBottomLine) * lineHeight;
	if (offset >= 0 && y < yMaxOffset) {
		offset += horBytes * ((y - frameT) / lineHeight);
	}
	return offset;
}

bool HexViewer::event(QEvent* e)
{
	if (e->type() != QEvent::ToolTip) {
		return QFrame::event(e);
	}

	// calculate address for tooltip
	auto* helpEvent = static_cast<QHelpEvent*>(e);
	int offset = coorToOffset(helpEvent->x(), helpEvent->y());
	if (offset >= 0 && (hexTopAddress + offset) < debuggableSize) {
		// create text with binary and decimal values
		int address = hexTopAddress + offset;
		unsigned char chr = hexData[address];
		QString text = QString("Address: %1").arg(QString("%1").arg(address, addressLength, 16, QChar('0')).toUpper());

		// print 8 bit values
		text += QString("\nBinary: %1 %2")
			.arg(chr >> 4, 4, 2, QChar('0'))
			.arg(chr & 0x000F, 4, 2, QChar('0'));
		text += QString("\nDecimal: %1").arg(chr);

		// print 16 bit values if possible
		if ((address + 1) < debuggableSize) {
			unsigned wd = chr;
			wd += 256 * hexData[address + 1];
			text += QString("\n\nWord: %1").arg(QString("%1").arg(wd, 4, 16, QChar('0')).toUpper());
			text += QString("\nBinary: %1 %2 %3 %4")
				.arg((wd & 0xF000) >> 12, 4, 2, QChar('0'))
				.arg((wd & 0x0F00) >>  8, 4, 2, QChar('0'))
				.arg((wd & 0x00F0) >>  4, 4, 2, QChar('0'))
				.arg((wd & 0x000F) >>  0, 4, 2, QChar('0'));
			text += QString("\nDecimal: %1").arg(wd);
		}
		QToolTip::showText(helpEvent->globalPos(), text);
	} else {
		QToolTip::hideText();
	}
	return QFrame::event(e);
}

void HexViewer::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton && isInteractive) {
		int offset=coorToOffset(e->x(), e->y());
		if (offset >= 0) {
			int addr = hexTopAddress + offset;
			if (useMarker && (hexMarkAddress != addr)) {
				setLocation(addr);
			} else {
				if (!useMarker) hexMarkAddress = addr;
				editValue = 0;
				cursorPosition = 0;
				beingEdited = isEditable;
			}
			editedChars = (e->x() >= xChar);
		}
		update();
	}
}

void HexViewer::focusInEvent(QFocusEvent* /*e*/)
{
	hasFocus = true;
	update();
}

void HexViewer::focusOutEvent(QFocusEvent* e)
{
	if (e->lostFocus()) {
		editValue = 0;
		cursorPosition = 0;
		beingEdited = false;
		hasFocus = false;
	}
	update();
}
