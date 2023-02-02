#include "VramTiledView.h"
#include "VDPDataStore.h"
#include "MSXPalette.h"
#include "Convert.h"
#include "ranges.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>

VramTiledView::VramTiledView(QWidget* parent)
	: QWidget(parent)
	, image(512, 512, QImage::Format_RGB32)
{
    palette = nullptr;
    setPaletteSource(VDPDataStore::instance().getPalette(paletteVDP));
	setZoom(1.0f);

	// Mouse update events when mouse is moved over the image, Quibus likes this
	// better than my preferred click-on-the-image.
	setMouseTracking(true);
}

void VramTiledView::setZoom(float zoom)
{
	zoomFactor = std::max(1.0f, zoom);
	if (vramBase) {
		const auto* regs = VDPDataStore::instance().getRegsPointer();
		lines = (forcedScreenRows != 0.0f) ? int(8 * forcedScreenRows) : (regs[9] & 128 ? 212 : 192);
	} else {
		lines = (forcedScreenRows != 0.0f) ? int(8 * forcedScreenRows) : 192;
	}
	int imageW = screenMode ==  0 ? 40 * 6 * 2
	           : screenMode == 80 ? 80 * 6
		                      : 512;
	setFixedSize(int(zoomFactor * float(imageW)),
	             int(zoomFactor * float(lines) * 2));
	update();
}

void VramTiledView::decode()
{
	if (!vramBase) return;

	//also everytime we decode we adjust height of widget to the correct screenheight
	const auto* regs = VDPDataStore::instance().getRegsPointer();
	lines = (forcedScreenRows != 0.0f) ? int(8 * forcedScreenRows) : (regs[9] & 128 ? 212 : 192);
	int imageW = screenMode ==  0 ? 40 * 6 * 2
	           : screenMode == 80 ? 80 * 6
		                      : 512;
	setFixedSize(int(zoomFactor * float(imageW)),
	             int(zoomFactor * float(lines) * 2));

	image.fill(Qt::gray);

	switch (tableToShow) {
		case 0:
			decodePatternTable();
			break;
		case 1:
			decodeNameTable();
			break;
		case 2:
			decodeNameTable();
			overLayNameTable();
			break;
	}

	horiStep = charWidth * ((screenMode == 80 && tableToShow > 0) ? 1 : 2);

	if (drawGrid && screenMode != 255) {
		QPainter qp(&image);
		qp.setPen(QColor(55, 55, 55, 128));
		for (int yy = 15; yy < 16 * screenHeight; yy += 16) {
			qp.drawLine(0, yy, horiStep * screenWidth - 1, yy);
		}
		for (int xx = horiStep - 1; xx < horiStep * screenWidth; xx += horiStep) {
			qp.drawLine(xx, 0, xx, 16*screenHeight - 1);
		}
	}

	pixImage = QPixmap::fromImage(image);
	update();
}


void VramTiledView::decodePatternTable()
{
	screenWidth = 32;
	screenHeight = 8; // 32 * 8 => 256 chars by default
	charWidth = 8;

	switch (screenMode) {
	case 0:
	case 80:
		charWidth = 6;
		decodePatternTableRegularChars();
		break;
	case 2:
	case 4:
		screenHeight = 24;
		[[fallthrough]];
	case 1:
		decodePatternTableRegularChars();
		break;
	case 3:
		// Since patterns in multicolor are 2 color nibles they act more like a 'colortable'
		// and to avoid too much special case in the generic decodePatternTableRegularChars()
		// this has been seperated in a single method.
		decodePatternTableMultiColor();
		return;
	default:
		warningImage();
	}
}

void VramTiledView::warningImage()
{
	QPainter qp(&image);
	qp.setPen(Qt::black);
	qp.drawText(16, 16, "Not a tile based screenmode!  Please use the 'Bitmapped VRAM' viewer.");
}

void VramTiledView::decodeNameTable()
{
	const auto* regs = VDPDataStore::instance().getRegsPointer();

	screenWidth = 32;
	charWidth = 8;
	screenHeight = (forcedScreenRows != 0.0f) ? int(forcedScreenRows + 0.5) : (regs[9] & 128 ? 27 : 24); // Check LN bit of vdp reg #9 to determine screenheight

	switch (screenMode) {
	case 0:
		screenWidth = 40;
		charWidth = 6;
		decodeNameTableRegularChars();
		break;
	case 80:
		screenWidth = 80;
		charWidth = 6;
		decodeNameTableRegularChars();
		break;
	case 4:
	case 1:
	case 2:
		decodeNameTableRegularChars();
		break;
	case 3:
		decodeNameTableMultiColor();
		return;
	default:
		warningImage();
	}

}

void VramTiledView::overLayNameTable()
{
	if (screenMode == 255) return; // No overlay if not tile based!

	// screenwidth, screenheight, charwidth are all calculated by decodeNameTable already...

	// Now draw the char numbers from the overlay over all the chars
	QImage overlayimage(":/overlay.png");
	QPainter qp(&image);
	int pWidth = screenMode != 80 ? 2 : 1;

	for (int charY = 0; charY < screenHeight; ++charY) {
		for (int charX = 0; charX < screenWidth; ++charX) {
			int ch = vramBase[nameTableAddress + charX + charY * screenWidth];
			int x = 6 * ((ch >> 4) & 15);
			int y = (ch == highlightChar ? 8 : 0); // White letters or red variant if highlighted
			qp.drawImage(pWidth * charX * charWidth, 16 * charY, overlayimage, x, y, 5, 8);
			x = 6 * (ch & 15);
			if (screenMode != 80) {
				qp.drawImage(pWidth * charX * charWidth + 6, charY * 16, overlayimage, x, y, 5, 8); // draw to the right
			} else {
				qp.drawImage(pWidth * charX * charWidth, charY * 16 + 7, overlayimage, x, y, 5, 8); // draw below
			}
		}
	}
}

void VramTiledView::setPixel1x2(int x, int y, QRgb c)
{
	image.setPixel(x, 2 * y + 0, c);
	image.setPixel(x, 2 * y + 1, c);
}

void VramTiledView::setPixel2x2(int x, int y, QRgb c)
{
	image.setPixel(2 * x + 0, 2 * y + 0, c);
	image.setPixel(2 * x + 1, 2 * y + 0, c);
	image.setPixel(2 * x + 0, 2 * y + 1, c);
	image.setPixel(2 * x + 1, 2 * y + 1, c);
}


QRgb VramTiledView::getColor(int c)
{
	// TODO do we need to look at the TP bit???
    return palette->color(c ? c : (tpBit ? 0 : borderColor));
}

int VramTiledView::getScreenMode() const
{
	return screenMode;
}

void VramTiledView::setHighlightChar(int value)
{
	if (highlightChar == value) return;

	highlightChar = value;
	decode();

	if (vramBase == nullptr || value < 0 || value > 255) return;

	// Count how many 'value'-characters there are in the nametable
	int count = 0;
	for (int i = 0; i < screenHeight * screenWidth; ++i) {
		if (vramBase[nameTableAddress + i] == value) {
			count++;
		}
	}
	emit highlightCount(value & 255, count);
}

void VramTiledView::setForcedScreenRows(float value)
{
	if (forcedScreenRows == value) return;

	forcedScreenRows = value;
	setZoom(zoomFactor); // Redo fixed size of widget
	decode();
}

void VramTiledView::setUseBlink(bool value)
{
	if (useBlink == value) return;
	useBlink = value;
	decode();
}

void VramTiledView::setTpBit(bool value)
{
	if (tpBit == value) return;
	tpBit = value;
	decode();
}

void VramTiledView::decodePatternTableRegularChars()
{
	for (int charY = 0; charY < screenHeight; ++charY) {
		for (int charX = 0; charX < screenWidth; ++charX) {
			drawCharAt(charX + charY * screenWidth, charX, charY);
		}
	}
}

void VramTiledView::decodePatternTableMultiColor()
{
	for (int charY = 0; charY < screenHeight; ++charY) {
		for (int charX = 0; charX < screenWidth; ++charX) {
			for (int charRow = 0; charRow < 8; ++charRow) {
				int ch = vramBase[patternTableAddress + 8 * (charX + charY * screenWidth) + charRow];
				QRgb col = getColor(ch >> 4);
				setPixel2x2(8 * charX + 0, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 1, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 2, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 3, 8 * charY + charRow, col);
				col = getColor(ch & 15);
				setPixel2x2(8 * charX + 4, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 5, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 6, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 7, 8 * charY + charRow, col);
			}
		}
	}
}

void VramTiledView::decodeNameTableRegularChars()
{
	for (int charY = 0; charY < screenHeight; ++charY) {
		for (int charX = 0; charX < screenWidth; ++charX) {
			int ch = vramBase[nameTableAddress + charX + charY * screenWidth];
			// special case screen2/screen4a since they have 3*256 chars
			if (charY > 7 && (screenMode == 2 || screenMode == 4)) {
				ch += 256 * (charY >> 3);
			}
			drawCharAt(ch, charX, charY);
		}
	}
}

void VramTiledView::decodeNameTableMultiColor()
{
	for (int charY = 0; charY < screenHeight; ++charY) {
		for (int charX = 0; charX < screenWidth; ++charX) {
			uint8_t ch = vramBase[nameTableAddress + charX + charY * screenWidth];
			for (int charRow = 0; charRow < 8; ++charRow) {
				uint8_t patternByte = vramBase[patternTableAddress + 8 * ch + 2 * (charY & 3) + (charRow >> 2)];
				QRgb col = getColor(patternByte >> 4);
				setPixel2x2(8 * charX + 0, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 1, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 2, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 3, 8 * charY + charRow, col);
				col = getColor(patternByte & 15);
				setPixel2x2(8 * charX + 4, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 5, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 6, 8 * charY + charRow, col);
				setPixel2x2(8 * charX + 7, 8 * charY + charRow, col);
			}
		}
	}
}

uint8_t VramTiledView::getCharColorByte(int character, int x, int y, int row)
{
	const auto* regs = VDPDataStore::instance().getRegsPointer();
	uint8_t colorByte = regs[7];
	switch (screenMode) {
	case 0:
		break;
	case 80:
		// If blink bit set then show alternate colors
		if (useBlink && (tableToShow > 0) &&
		    (vramBase[colorTableAddress + (x >> 3) + 10 * y] & (1 << (7 - (7 & x))))) {
			colorByte = regs[12];
		}
		break;
	case 2:
	case 4:
		colorByte = vramBase[colorTableAddress + 8 * character + row];
		break;
	case 1:
		colorByte = vramBase[colorTableAddress + (character >> 3)];
		break;
	case 3:
		//decodePatternTableMultiColor();
		colorByte = 0xf0;
	}
	return colorByte;
}

void VramTiledView::drawCharAt(int character, int x, int y)
{
	for (int charRow = 0; charRow < 8; ++charRow) {
		uint8_t patternByte = vramBase[patternTableAddress + 8 * character + charRow];
		for (int charCol = 0; charCol < charWidth; ++charCol) {
			uint8_t mask = 1 << (7 - charCol);
			uint8_t colors = getCharColorByte(character, x, y, charRow);
			QRgb col = getColor(patternByte & mask ? (colors >> 4) : (colors & 15));
			if (screenMode == 80 && tableToShow > 0) {
				setPixel1x2(x * charWidth + charCol, 8 * y + charRow, col);
			} else {
				setPixel2x2(x * charWidth + charCol, 8 * y + charRow, col);
			}
		}
	}
}

void VramTiledView::drawCharAtImage(int character, int x, int y, QImage& image)
{
	QPainter qp(&image);
	qp.setPen(Qt::NoPen);
	for (int charRow = 0; charRow < 8; ++charRow) {
		uint8_t patternByte = vramBase[patternTableAddress + 8 * character + charRow];
		for (int charCol = 0; charCol < 8; ++charCol) { // Always draw the 8 bits even for screen 0
			uint8_t mask = 1 << (7 - charCol);
			uint8_t colors = getCharColorByte(character, x, y, charRow);
			QRgb col = getColor(patternByte & mask ? (colors >> 4) : (colors & 15));
			// local setPixel4x4 routine  :-)
			qp.setBrush(QColor(col));
			qp.drawRect(4 * charCol, 4 * charRow, 4, 4);
		}
	}
}

void VramTiledView::setTabletoShow(int value)
{
	if (tableToShow == value) return;
	tableToShow = value;
	decode();
}

void VramTiledView::setDrawGrid(bool value)
{
	if (drawGrid == value) return;
	drawGrid = value;
	decode();
}

unsigned VramTiledView::getColorTableAddress() const
{
	return colorTableAddress;
}

unsigned VramTiledView::getPatternTableAddress() const
{
	return patternTableAddress;
}

unsigned VramTiledView::getNameTableAddress() const
{
	return nameTableAddress;
}

void VramTiledView::paintEvent(QPaintEvent* /*event*/)
{
	QRect srcRect(0, 0, 512, 2 * lines);
	QRect dstRect(0, 0, int(512 * zoomFactor), int(2 * float(lines) * zoomFactor));
	QPainter qp(this);
	//qp.drawImage(rect(), image, srcRect);
	qp.drawPixmap(dstRect, pixImage, srcRect);
}

void VramTiledView::refresh()
{
    decode();
}

QString VramTiledView::byteAsPattern(uint8_t byte)
{
	QString val;
	for (int i = 7; i >= 0; --i) {
		val.append(QChar(byte & (1 << i) ? '1' : '.'));
	}
	val.append(QString(" %1").arg(hexValue(byte, 2)));
	return val;
}

QString VramTiledView::textInfo(int x, int y, int character)
{
	QString info("Generator Data         Color data\n");

	// Now the initial color data to be displayed
	const auto* regs = VDPDataStore::instance().getRegsPointer();
	QString colordata = QString("- VDP reg 7 is %1").arg(hexValue(regs[7], 2));
	// If blink bit set then show alternate colors
	if (screenMode == 80 && useBlink && (tableToShow > 0) &&
	    (vramBase[colorTableAddress + (x >> 3) + 10 * y] & (1 << (7 - (7 & x)))))
	{
		colordata = QString("- VDP reg 12 is %1").arg(hexValue(regs[12], 2));
	} else if (screenMode == 1) {
		colordata = QString("- %1: %2").arg(
			hexValue(colorTableAddress + (character >> 3), 4),
			hexValue(vramBase[colorTableAddress + (character >> 3)], 2));
	} else if (screenMode == 3) {
		colordata = QString("- not used");
	}

	// Now build the text
	for (int charRow = 0; charRow < 8; ++charRow) {
		int addr = patternTableAddress + 8 * character + charRow;
		// Color data changes per line for screen 2 and 4
		if (screenMode == 2 || screenMode == 4) {
			colordata = QString("  %1: %2").arg(
				hexValue(colorTableAddress + 8 * character + charRow, 4),
				hexValue(vramBase[colorTableAddress + 8 * character + charRow], 2));
		}

		info.append(QString("%1: %2    %3\n").arg(
			hexValue(addr, 4),
			byteAsPattern(vramBase[addr]),
			colordata));
		colordata.clear();
	}
	return info;
}

void VramTiledView::mousePressEvent(QMouseEvent* e)
{
	if (auto i = infoFromMouseEvent(e)) {
		emit imageClicked(i->x, i->y, i->character, textInfo(i->x, i->y, i->character));
	}
}

void VramTiledView::mouseMoveEvent(QMouseEvent* e)
{
	if (auto i = infoFromMouseEvent(e)) {
		emit imageHovered(i->x, i->y, i->character);
	}
}

std::optional<VramTiledView::MouseEventInfo> VramTiledView::infoFromMouseEvent(QMouseEvent* e)
{
	if (!vramBase) return {};

	// I see negative y-coords sometimes, so for safety clip the coords
	int x = std::clamp(int(float(e->x()) / zoomFactor),     0, 511) / horiStep;
	int y = std::clamp(int(float(e->y()) / zoomFactor) / 2, 0, 255) / 8;

	if (x >= screenWidth) return {};
	if (y >= screenHeight) return {};

	x = int(x / horiStep);
	y = int(y / 8);
	int character = 0;

	switch (tableToShow) {
		case 0:
			character = x + y * screenWidth;
			if (!(screenMode == 2 || screenMode == 4)) {
				character &= 255;
			}
			break;
		case 1:
		case 2:
			character = vramBase[nameTableAddress + x + y * screenWidth];
			if (screenMode == 2 || screenMode == 4) {
				character += 256 * int(y / 8);
			}
			break;
	}
	return MouseEventInfo{x, y, character};
}

void VramTiledView::setBorderColor(int value)
{
	borderColor = std::clamp(value, 0, 15);
	refresh();
}

void VramTiledView::setScreenMode(int mode)
{
	screenMode = mode;
	decode();
}

void VramTiledView::setVramSource(const uint8_t* adr)
{
	vramBase = adr;
	refresh();
}

void VramTiledView::setNameTableAddress(int adr)
{
	nameTableAddress = adr;
	decode();
}

void VramTiledView::setPatternTableAddress(int adr)
{
	patternTableAddress = adr;
	decode();
}

void VramTiledView::setColorTableAddress(int adr)
{
	colorTableAddress = adr;
	decode();
}

void VramTiledView::setPaletteSource(MSXPalette *adr)
{
	if (palette == adr) return;
    if (palette) {
        disconnect(palette, &MSXPalette::paletteChanged, this, &VramTiledView::decode);
    }
	palette = adr;
    connect(palette, &MSXPalette::paletteChanged, this, &VramTiledView::decode);
	refresh();
}
