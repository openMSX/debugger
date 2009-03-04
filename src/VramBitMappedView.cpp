#include "VramBitMappedView.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>

/** Clips x to the range [LO,HI].
  * Slightly faster than    std::min(HI, std::max(LO, x))
  * especially when no clipping is required.
  */
template <int LO, int HI>
static inline int clip(int x)
{
	return unsigned(x - LO) <= unsigned(HI - LO) ? x : (x < HI ? LO : HI);
}


VramBitMappedView::VramBitMappedView(QWidget* parent)
	: QWidget(parent)
	, image(512, 512, QImage::Format_RGB32)
{
	lines = 212;
	screenMode = 5;
	borderColor = 0;
	pallet = NULL;
	vramBase = NULL;
	vramAddress = 0;
	for (int i = 0; i < 15; ++i) {
		msxpallet[i] = qRgb(80, 80, 80);
	}
	setZoom(1.0f);

	// mouse update events when mouse is moved over the image, Quibus likes this
	// better then my preferd click-on-the-image
	setMouseTracking(true);
}

void VramBitMappedView::setZoom(float zoom)
{
	zoomFactor = std::max(1.0f, zoom);
	setFixedSize(int(512 * zoomFactor), int(lines * 2 * zoomFactor));
	update();
}

void VramBitMappedView::decode()
{
	if (!vramBase) return;

	printf("\n"
	       "screenMode: %i\n"
	       "vram to start decoding: %i\n",
	       screenMode, vramAddress);
	switch (screenMode) {
	case 12:
		decodeSCR12();
		break;
	case 11:
	case 10:
		decodeSCR10();
		break;
	case 8:
		decodeSCR8();
		break;
	case 7:
		decodeSCR7();
		break;
	case 6:
		decodeSCR6();
		break;
	case 5:
		decodeSCR5();
		break;
	}
	piximage = piximage.fromImage(image);
	update();
}

void VramBitMappedView::decodePallet()
{
	if (!pallet) return;

	for (int i = 0; i < 16; ++i) {
		int r = (pallet[2 * i + 0] & 0xf0) >> 4;
		int b = (pallet[2 * i + 0] & 0x0f);
		int g = (pallet[2 * i + 1] & 0x0f);

		r = (r >> 1) | (r << 2) | (r << 5);
		b = (b >> 1) | (b << 2) | (b << 5);
		g = (g >> 1) | (g << 2) | (g << 5);

		msxpallet[i] = qRgb(r, g, b);
	}
}

static unsigned interleave(unsigned x)
{
	return (x >> 1) | ((x & 1) << 16);
}

void VramBitMappedView::setPixel2x2(int x, int y, QRgb c)
{
	image.setPixel(2 * x + 0, 2 * y + 0, c);
	image.setPixel(2 * x + 1, 2 * y + 0, c);
	image.setPixel(2 * x + 0, 2 * y + 1, c);
	image.setPixel(2 * x + 1, 2 * y + 1, c);
}
void VramBitMappedView::setPixel1x2(int x, int y, QRgb c)
{
	image.setPixel(x, 2 * y + 0, c);
	image.setPixel(x, 2 * y + 1, c);
}

void VramBitMappedView::decodeSCR12()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 4) {
			unsigned p[4];
			p[0] = vramBase[interleave(offset++)];
			p[1] = vramBase[interleave(offset++)];
			p[2] = vramBase[interleave(offset++)];
			p[3] = vramBase[interleave(offset++)];
			int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
			int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);

			for (unsigned n = 0; n < 4; ++n) {
				int z = p[n] >> 3;
				int r = clip<0, 31>(z + j);
				int g = clip<0, 31>(z + k);
				int b = clip<0, 31>((5 * z - 2 * j - k) / 4);
				r = (r << 3) | (r >> 2);
				b = (b << 3) | (b >> 2);
				g = (g << 3) | (g >> 2);
				setPixel2x2(x + n, y, qRgb(r, g, b));
			}
		}
	}
}

void VramBitMappedView::decodeSCR10()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 4) {
			unsigned p[4];
			p[0] = vramBase[interleave(offset++)];
			p[1] = vramBase[interleave(offset++)];
			p[2] = vramBase[interleave(offset++)];
			p[3] = vramBase[interleave(offset++)];
			int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
			int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);

			for (unsigned n = 0; n < 4; ++n) {
				QRgb c;
				if (p[n] & 0x08) {
					// YAE
					c = msxpallet[p[n] >> 4];
				} else {
					// YJK
					int z = p[n] >> 3;
					int r = clip<0, 31>(z + j);
					int g = clip<0, 31>(z + k);
					int b = clip<0, 31>((5 * z - 2 * j - k) / 4);
					r = (r << 3) | (r >> 2);
					b = (b << 3) | (b >> 2);
					g = (g << 3) | (g >> 2);
					c = qRgb(r, g, b);
				}
				setPixel2x2(x + n, y, c);
			}
		}
	}
}

void VramBitMappedView::decodeSCR8()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; ++x) {
			unsigned char val = vramBase[interleave(offset++)];
			int b = val & 0x03;
			int r = val & 0x1C;
			int g = val & 0xE0;

			b = b | (b << 2) | (b << 4) | (b << 6);
			r = (r >> 2) | r | (r << 3);
			g = g | (g >> 3) | (g >> 6);

			setPixel2x2(x, y, qRgb(r, g, b));
		}
	}
}

QRgb VramBitMappedView::getColor(int c)
{
	// TODO do we need to look at the TP bit???
	return msxpallet[c ? c : borderColor];
}

void VramBitMappedView::decodeSCR7()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 512; x += 2) {
			int val = vramBase[interleave(offset++)];
			setPixel1x2(x + 0, y, getColor((val >> 4) & 15));
			setPixel1x2(x + 1, y, getColor((val >> 0) & 15));
		}
	}
}

void VramBitMappedView::decodeSCR6()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 512; x += 4) {
			int val = vramBase[offset++];
			setPixel1x2(x + 0, y, getColor((val >> 6) & 3));
			setPixel1x2(x + 1, y, getColor((val >> 4) & 3));
			setPixel1x2(x + 2, y, getColor((val >> 2) & 3));
			setPixel1x2(x + 3, y, getColor((val >> 0) & 3));
		}
	}
}

void VramBitMappedView::decodeSCR5()
{
	int offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 2) {
			int val = vramBase[offset++];
			setPixel2x2(x + 0, y, getColor((val >> 4) & 15));
			setPixel2x2(x + 1, y, getColor((val >> 0) & 15));
		}
	}
}

void VramBitMappedView::paintEvent(QPaintEvent* event)
{
	QRect srcRect(0, 0, 512, 2 * lines);
	QRect dstRect(0, 0, int(512 * zoomFactor), int(2 * lines * zoomFactor));
	QPainter qp(this);
	//qp.drawImage(rect(),image,srcRect);
	qp.drawPixmap(dstRect, piximage, srcRect);
}

void VramBitMappedView::refresh()
{
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::mouseMoveEvent(QMouseEvent* e)
{
	static const unsigned bytes_per_line[] = {
		0,	//screen 0
		1,	//screen 1
		2,	// 2
		3,	// 3
		4,	
		128,	// 5
		128,
		256,	// 7
		256,
		256,	// 9
		256,
		256,
		256
	};
	static const unsigned pixels_per_byte[] = {
		0,1,2,3,4,
		2,	// 5
		4,
		2,	// 7
		1,
		1,	//9
		1,
		1,
		1
	};

	int x = int(e->x() / zoomFactor);
	int y = int(e->y() / zoomFactor) >> 1;
	if ((screenMode != 6) && (screenMode != 7)) {
		x /= 2;
	}

	unsigned offset = bytes_per_line[screenMode] * y
	                + x / pixels_per_byte[screenMode];
	unsigned addr = offset + vramAddress;
	int val = (screenMode >= 7) ? vramBase[interleave(addr)]
	                            : vramBase[addr];

	int color;
	switch (screenMode) {
		case 5:
		case 7:
			color = ((x & 1) ? val : (val >> 4)) & 15;
			break;
		case 6:
			color = (val >> (2 * (3 - (x & 3)))) & 3;
			break;
		case 8:
		case 10:
		case 11:
		case 12:
			color = val;
			break;
		default:
			color = 0; // avoid warning
	}
	emit imagePosition(x, y, color, addr, val);
}

void VramBitMappedView::mousePressEvent(QMouseEvent* e)
{
	// since mouseMove only emits the correct signal we reuse/abuse that method
	mouseMoveEvent(e);

	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setBorderColor(int value)
{
	borderColor = clip<0, 15>(value);
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setScreenMode(int mode)
{
	screenMode = mode;
	decode();
	update();
}

void VramBitMappedView::setLines(int nrLines)
{
	lines = nrLines;
	decode();
	setFixedSize(int(512 * zoomFactor), int(lines * 2 * zoomFactor));
	update();
	//setZoom(zoomFactor);
}

void VramBitMappedView::setVramAddress(int adr)
{
	vramAddress = adr;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setVramSource(const unsigned char* adr)
{
	vramBase = adr;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setPaletteSource(const unsigned char* adr)
{
	pallet = adr;
	decodePallet();
	decode();
	update();
}
