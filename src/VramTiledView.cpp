#include "VramTiledView.h"
#include "VDPDataStore.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>
#include "Convert.h"

/** Clips x to the range [LO, HI].
  * Slightly faster than    std::min(HI, std::max(LO, x))
  * especially when no clipping is required.
  */
template <int LO, int HI>
static inline int clip(int x)
{
	return unsigned(x - LO) <= unsigned(HI - LO) ? x : (x < HI ? LO : HI);
}


VramTiledView::VramTiledView(QWidget* parent)
	: QWidget(parent)
	, image(512, 512, QImage::Format_RGB32)
{
	lines = 212;
	screenMode = 0;
	tableToShow = 0;
	borderColor = 0;
	pallet = nullptr;
	vramBase = nullptr;
	PatternTableAddress = 0;
	ColorTableAddress = 0;
	NameTableAddress = 0;
	drawgrid = true;
	useBlink = false;
	TPbit = false;
	forcedscreenrows = 0;
	highlightchar = -1; //anythin outside the 0-255 range will do :-)

	for (int i = 0; i < 15; ++i) {
		msxpallet[i] = qRgb(80, 80, 80);
	}
	setZoom(1.0f);

	// mouse update events when mouse is moved over the image, Quibus likes this
	// better then my preferd click-on-the-image
	setMouseTracking(true);
}

void VramTiledView::setZoom(float zoom)
{
	zoomFactor = std::max(1.0f, zoom);
	if (vramBase!=nullptr) {
		const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
		lines = forcedscreenrows ? (forcedscreenrows*8) : (regs[9] & 128 ? 212 : 192);
	} else {
		lines = forcedscreenrows ? (forcedscreenrows*8) : 192;
	}
	int imagew = screenMode == 0 ? 40*6*2 : (screenMode == 80 ? 80*6 : 512);
	setFixedSize(int(imagew * zoomFactor), int(lines * 2 * zoomFactor));
	update();
}

void VramTiledView::decode()
{
	if (!vramBase) return;

	//also everytime we decode we adjust height of widget to the correct screenheight
	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
	lines = forcedscreenrows ? (forcedscreenrows*8) : (regs[9] & 128 ? 212 : 192);
	int imagew = screenMode == 0 ? 40*6*2 : (screenMode == 80 ? 80*6 : 512);
	setFixedSize(int(imagew * zoomFactor), int(lines * 2 * zoomFactor));

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

	horistep = charwidth*((screenMode == 80 && tableToShow > 0) ? 1 : 2);

	if (drawgrid && screenMode != 255) {
		QPainter qp(&image);
		qp.setPen(QColor(55, 55, 55, 128));
		for (int yy = 15; yy < 16*screenheight; yy += 16) {
			qp.drawLine(0, yy, horistep*screenwidth - 1, yy);
		}
		for (int xx = horistep - 1; xx < horistep*screenwidth; xx += horistep) {
			qp.drawLine(xx, 0, xx, 16*screenheight - 1);
		}
	};

	piximage = QPixmap::fromImage(image);
	update();
}

void VramTiledView::decodePallet()
{
	if (!pallet) return;
    //printf("VramTiledView::decodePallet  palletpointer %p \n",pallet);

	for (int i = 0; i < 16; ++i) {
		int r = (pallet[2 * i + 0] & 0xf0) >> 4;
		int b = (pallet[2 * i + 0] & 0x0f);
		int g = (pallet[2 * i + 1] & 0x0f);

		r = (r >> 1) | (r << 2) | (r << 5);
		b = (b >> 1) | (b << 2) | (b << 5);
		g = (g >> 1) | (g << 2) | (g << 5);

		msxpallet[i] = qRgb(r, g, b);
        //printf("VramTiledView::decodePallet  color %d => r %d  g %d  b %d \n",i,r,g,b);
	}
}

void VramTiledView::decodePatternTable()
{
//	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();

	screenwidth = 32;
	screenheight = 8; // 32*8 => 256 chars by default
	charwidth = 8;

	switch (screenMode) {
	case 0:
	case 80:
		charwidth = 6;
		decodePatternTableRegularChars();
		break;
	case 2:
	case 4:
		screenheight = 24;
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
	};
}

void VramTiledView::warningImage()
{
	QPainter qp(&image);
	qp.setPen(Qt::black);
	qp.drawText(16, 16, QString("Not a tile based screenmode!  Please use the 'Bitmapped VRAM' viewer."));
}


void VramTiledView::decodeNameTable()
{
	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();

	screenwidth = 32;
	charwidth = 8;
	screenheight = forcedscreenrows ? int(forcedscreenrows + 0.5) : (regs[9] & 128 ? 27 : 24) ; // check LN bit of vdp reg #9 to determine screenheight

	switch (screenMode) {
	case 0:
		screenwidth = 40;
		charwidth = 6;
		decodeNameTableRegularChars();
		break;
	case 80:
		screenwidth = 80;
		charwidth = 6;
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
	};

}

void VramTiledView::overLayNameTable()
{
	if (screenMode == 255) return; // No overlay if not tile based!

	// screenwidth, screenheight, charwidth are all calculated by decodeNameTable already...


	//now draw the char numbers from the overlay over all the chars
	QImage overlayimage(":/overlay.png");
	QPainter qp(&image);
	int pwidth = screenMode!=80?2:1;

	for (int chary = 0; chary < screenheight ; chary++) {
		for (int charx = 0; charx < screenwidth ; charx++) {
			int ch = vramBase[NameTableAddress + charx + chary*screenwidth];
			int x = ((ch >> 4) & 15)*6;
			int y = (ch == highlightchar ? 8 : 0); // white letters or red variant if higlighted
			qp.drawImage(pwidth * charx * charwidth, chary*16, overlayimage, x, y, 5, 8);
			x = (ch & 15)*6;
			if (screenMode != 80) {
				qp.drawImage(pwidth * charx * charwidth + 6, chary * 16, overlayimage, x, y, 5, 8); //draw to the right
			} else {
				qp.drawImage(pwidth * charx * charwidth, chary * 16 + 7, overlayimage, x, y, 5, 8); //draw below
			};
		}
	}

}

//static unsigned interleave(unsigned x)
//{
//	return (x >> 1) | ((x & 1) << 16);
//}


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
	return msxpallet[c ? c : (TPbit ? 0 : borderColor)];
}

const unsigned char *VramTiledView::getPaletteSource() const
{
	return pallet;
}

int VramTiledView::getScreenMode() const
{
	return screenMode;
}

void VramTiledView::setHighlightchar(int value)
{
	if (highlightchar == value) return;

	highlightchar = value;
	decode();

	if (vramBase == nullptr || value < 0 || value > 255) return;

	//count how many 'value'-characters there are in the nametable
	int count = 0;
	for (int i = 0; i<screenheight*screenwidth ; i++) {
		if (vramBase[NameTableAddress+i] == value) {
			count++;
		}
	}
	emit highlightCount(value & 255, count);
}

void VramTiledView::setForcedscreenrows(float value)
{
	if (forcedscreenrows == value) return ;

	forcedscreenrows = value;
	setZoom(zoomFactor); // redo fixed size of widget
	decode();
}

void VramTiledView::setUseBlink(bool value)
{
	if (useBlink == value) return;

	useBlink = value;
	decode();
}

void VramTiledView::setTPbit(bool value)
{
	if (TPbit == value) return;

	TPbit = value;
	decode();
}


void VramTiledView::decodePatternTableRegularChars()
{
	for (int chary = 0; chary < screenheight ; chary++) {
		for (int charx = 0; charx < screenwidth ; charx++) {
			drawCharAt(charx + chary*screenwidth, charx, chary);
		}
	}
}

void VramTiledView::decodePatternTableMultiColor()
{
	for (int chary = 0; chary < screenheight ; chary++) {
		for (int charx = 0; charx < screenwidth ; charx++) {
			for (int charrow = 0 ; charrow < 8 ; charrow++) {
				int ch = vramBase[PatternTableAddress + 8 * (charx + chary*screenwidth) + charrow];
				QRgb col = getColor(ch >> 4);
				setPixel2x2(charx*8 + 0, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 1, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 2, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 3, chary*8 + charrow, col);
				col = getColor(ch & 15);
				setPixel2x2(charx*8 + 4, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 5, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 6, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 7, chary*8 + charrow, col);
			}
		}
	}
}

void VramTiledView::decodeNameTableRegularChars()
{
	for (int chary = 0; chary < screenheight ; chary++) {
		for (int charx = 0; charx < screenwidth ; charx++) {
			int ch = vramBase[NameTableAddress + charx + chary*screenwidth];
			//special case screen2/screen4a since they have 3*256 chars
			if (chary > 7 && (screenMode == 2 || screenMode == 4)) {
				ch = ch + 256*(chary >> 3);

			}
			drawCharAt(ch, charx, chary);
		}
	}
}

void VramTiledView::decodeNameTableMultiColor()
{
	for (int chary = 0; chary < screenheight ; chary++) {
		for (int charx = 0; charx < screenwidth ; charx++) {
			unsigned char ch = vramBase[NameTableAddress + charx + chary*screenwidth];
			for (int charrow = 0 ; charrow<8 ; charrow++) {
				unsigned char patternbyte = vramBase[PatternTableAddress + 8*ch + 2*(chary & 3) + (charrow >> 2)];
				QRgb col = getColor(patternbyte >> 4);
				setPixel2x2(charx*8 + 0, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 1, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 2, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 3, chary*8 + charrow, col);
				col = getColor(patternbyte & 15);
				setPixel2x2(charx*8 + 4, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 5, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 6, chary*8 + charrow, col);
				setPixel2x2(charx*8 + 7, chary*8 + charrow, col);
			}
		}
	}
}

unsigned char VramTiledView::getCharColorByte(int character, int x, int y, int row)
{
	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
	unsigned char colorbyte = regs[7];
	switch (screenMode) {
	case 0:
		break;
	case 80:
		// if blink bit set then show alternate colors
		if ( useBlink && (tableToShow > 0) &&
			 (vramBase[ColorTableAddress + (x >> 3) + y*10] & (1 << (7 - (7 & x))) ))
		{
			colorbyte = regs[12];
		}
		break;
	case 2:
	case 4:
		colorbyte = vramBase[ColorTableAddress + character*8 + row];
		break;
	case 1:
		colorbyte = vramBase[ColorTableAddress + (character >> 3)];
		break;
	case 3:
//		decodePatternTableMultiColor();
		colorbyte = 0xf0;
	}
	return colorbyte;
}

void VramTiledView::drawCharAt(int character, int x, int y)
{
	for (int charrow = 0 ; charrow < 8 ; charrow++) {
		unsigned char patternbyte = vramBase[PatternTableAddress + 8*character + charrow];
		for (int charcol = 0 ; charcol < charwidth ; charcol++) {
			unsigned char mask = 1 << (7 - charcol);
			unsigned char colors = getCharColorByte(character, x, y, charrow);
			QRgb col = getColor(patternbyte&mask ? (colors >> 4) : (colors & 15) );
			if (screenMode == 80 && tableToShow > 0) {
				setPixel1x2(x*charwidth+charcol, y*8 + charrow, col);
			} else {
				setPixel2x2(x*charwidth+charcol, y*8 + charrow, col);
			}
		}
	}
}

void VramTiledView::drawCharAtImage(int character, int x, int y, QImage &image)
{
	QPainter qp(&image);
	qp.setPen(Qt::NoPen);
	for (int charrow = 0 ; charrow < 8 ; charrow++) {
		unsigned char patternbyte = vramBase[PatternTableAddress + 8*character + charrow];
		for (int charcol = 0 ; charcol < 8 ; charcol++) { // always draw the 8 biits even for screen 0
			unsigned char mask = 1 << (7 - charcol);
			unsigned char colors = getCharColorByte(character, x, y, charrow);
			QRgb col = getColor(patternbyte & mask ? (colors >> 4) : (colors & 15) );
			// local setPixel4x4 routine  :-)
			qp.setBrush(QColor(col));
			qp.drawRect(4*charcol, 4*charrow, 4, 4);
		}
	}
}

void VramTiledView::setTabletoShow(int value)
{
	if (tableToShow == value) return ;
	tableToShow = value;
	decode();
}

void VramTiledView::setDrawgrid(bool value)
{
	if (drawgrid == value) return;
	drawgrid = value;
	decode();
}

unsigned VramTiledView::getColorTableAddress() const
{
	return ColorTableAddress;
}

unsigned VramTiledView::getPatternTableAddress() const
{
	return PatternTableAddress;
}

unsigned VramTiledView::getNameTableAddress() const
{
	return NameTableAddress;
}

void VramTiledView::paintEvent(QPaintEvent* /*event*/)
{
	QRect srcRect(0, 0, 512, 2 * lines);
	QRect dstRect(0, 0, int(512 * zoomFactor), int(2 * lines * zoomFactor));
	QPainter qp(this);
	//qp.drawImage(rect(), image, srcRect);
	qp.drawPixmap(dstRect, piximage, srcRect);
}

void VramTiledView::refresh()
{
	decodePallet();
	decode();
}

QString VramTiledView::byteAsPattern(unsigned char byte)
{
	QString val;
	for (int i = 7; i >= 0 ; i--) {
		val.append(QChar(byte & (1 << i) ? '1' : '.'));
	};
	val.append(QString(" %1").arg(hexValue(byte, 2)));
	return val;
}

QString VramTiledView::textinfo(int &x, int &y, int &character)
{
	QString info("Generator Data         Color data\n");

	//now the initial color data to be displayed
	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
	QString colordata = QString("- VDP reg 7 is %1").arg(hexValue(regs[7], 2));
	// if blink bit set then show alternate colors
	if ( screenMode == 80 && useBlink && (tableToShow > 0) &&
		 (vramBase[ColorTableAddress + (x >> 3) + y*10] & (1 << (7 - (7 & x))) ))
	{
		colordata = QString("- VDP reg 12 is %1").arg(hexValue(regs[12], 2));
	}
	if ( screenMode == 1)
	{
		colordata = QString("- %1: %2").arg(
					hexValue(ColorTableAddress + (character >> 3), 4) ,
					hexValue(vramBase[ColorTableAddress+(character >> 3)], 2)
				);
	};
	if ( screenMode == 3)
	{
		colordata = QString("- not used");
	};

	//now build the text
	for (int charrow = 0 ; charrow < 8; charrow++) {
		int addr = PatternTableAddress + 8*character + charrow;
		//color data changes per line for scrren 2 and 4
		if (screenMode == 2 || screenMode == 4) {
			colordata = QString("  %1: %2").arg(
			          hexValue(ColorTableAddress + character*8 + charrow, 4),
				  hexValue(vramBase[ColorTableAddress + character*8 + charrow], 2)
				);

		}

		info.append(QString("%1: %2    %3\n").arg(
					hexValue(addr, 4),
					byteAsPattern(vramBase[addr]),
					colordata
					)
				);
		colordata.clear();
	};
	return info;
}

void VramTiledView::mousePressEvent(QMouseEvent* e)
{
	int x = 0;
	int y = 0;
	int character = 0;
	if (infoFromMouseEvent(e, x, y, character)) {
		emit imageClicked(x, y, character, textinfo(x, y, character));
	};
}


void VramTiledView::mouseMoveEvent(QMouseEvent* e)
{
	int x = 0;
	int y = 0;
	int character = 0;
	if (infoFromMouseEvent(e, x, y, character)) {
		emit imageHovered(x, y, character);
	};
}

bool VramTiledView::infoFromMouseEvent(QMouseEvent* e, int &x, int &y, int &character)
{

	x = int(e->x() / zoomFactor);
	y = int(e->y() / zoomFactor) / 2;

	// I see negative y-coords sometimes, so for safety clip the coords
	x = std::max(0, std::min(511, x));
	y = std::max(0, std::min(255, y));

	if (x >= horistep*screenwidth || y >= 8*screenheight || vramBase == nullptr) {
		return false;
	}

	x = int(x/horistep);
	y = int(y/8);
	character = 0;
	switch (tableToShow) {
		case 0:
			character = x + y*screenwidth;
			if (!(screenMode == 2 || screenMode == 4)) {
				character = 255 & character;
			}
			break;
		case 1:
		case 2:
			character = vramBase[NameTableAddress + x + y*screenwidth];
			if (screenMode == 2||screenMode == 4) {
				character += 256*int(y/8);
			}
			break;
	};
	return true;
}


void VramTiledView::setBorderColor(int value)
{
	borderColor = clip<0, 15>(value);
    refresh();
}

void VramTiledView::setScreenMode(int mode)
{
	screenMode = mode;
	decode();
}

void VramTiledView::setVramSource(const unsigned char* adr)
{
	vramBase = adr;
    refresh();
}

void VramTiledView::setNameTableAddress(int adr)
{
	NameTableAddress = adr;
	decode();
}

void VramTiledView::setPatternTableAddress(int adr)
{
	PatternTableAddress = adr;
	decode();
}

void VramTiledView::setColorTableAddress(int adr)
{
	ColorTableAddress = adr;
	decode();
}

void VramTiledView::setPaletteSource(const unsigned char* adr)
{
	if (pallet == adr) return ;
	pallet = adr;
    refresh();
}
