#include "VramBitMappedView.h"
#include "ranges.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>

VramBitMappedView::VramBitMappedView(QWidget* parent)
	: QWidget(parent)
	, image(512, 512, QImage::Format_RGB32)
{
	ranges::fill(msxPalette, qRgb(80, 80, 80));
	setZoom(1.0f);

	// Mouse update events when mouse is moved over the image, Quibus likes this
	// better than my preferred click-on-the-image.
	setMouseTracking(true);
}

void VramBitMappedView::setZoom(float zoom)
{
	zoomFactor = std::max(1.0f, zoom);
	setFixedSize(int(512.0f * zoomFactor), int(float(lines) * 2.0f * zoomFactor));
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
		case  5: decodeSCR5();  break;
		case  6: decodeSCR6();  break;
		case  7: decodeSCR7();  break;
		case  8: decodeSCR8();  break;
		case 10:
		case 11: decodeSCR10(); break;
		case 12: decodeSCR12(); break;
	}
	pixImage = QPixmap::fromImage(image);
	update();
}

void VramBitMappedView::decodePallet()
{
	if (!palette) return;

	for (int i = 0; i < 16; ++i) {
		int r = (palette[2 * i + 0] & 0xf0) >> 4;
		int b = (palette[2 * i + 0] & 0x0f);
		int g = (palette[2 * i + 1] & 0x0f);
		auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
		msxPalette[i] = qRgb(scale(r), scale(g), scale(b));
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

static QRgb decodeYJK(int y, int j, int k)
{
	int r = std::clamp(y + j, 0, 31);
	int g = std::clamp(y + k, 0, 31);
	int b = std::clamp((5 * y - 2 * j - k) / 4, 0, 31);
	auto scale = [](int x) { return (x << 3) | (x >> 2); };
	return qRgb(scale(r), scale(g), scale(b));
}

void VramBitMappedView::decodeSCR12()
{
	auto offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 4) {
			uint8_t p[4];
			p[0] = vramBase[interleave(offset++)];
			p[1] = vramBase[interleave(offset++)];
			p[2] = vramBase[interleave(offset++)];
			p[3] = vramBase[interleave(offset++)];
			int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
			int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);
			for (int n = 0; n < 4; ++n) {
				setPixel2x2(x + n, y, decodeYJK(p[n] >> 3, j, k));
			}
		}
	}
}

void VramBitMappedView::decodeSCR10()
{
	auto offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 4) {
			uint8_t p[4];
			p[0] = vramBase[interleave(offset++)];
			p[1] = vramBase[interleave(offset++)];
			p[2] = vramBase[interleave(offset++)];
			p[3] = vramBase[interleave(offset++)];
			int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
			int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);
			for (int n = 0; n < 4; ++n) {
				QRgb c = (p[n] & 0x08) ? msxPalette[p[n] >> 4] // YAE
				                       : decodeYJK(p[n] >> 3, j, k); // YJK
				setPixel2x2(x + n, y, c);
			}
		}
	}
}

void VramBitMappedView::decodeSCR8()
{
	auto offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; ++x) {
			uint8_t val = vramBase[interleave(offset++)];
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
	return msxPalette[c ? c : borderColor];
}

void VramBitMappedView::decodeSCR7()
{
	auto offset = vramAddress;
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
	auto offset = vramAddress;
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
	auto offset = vramAddress;
	for (int y = 0; y < lines; ++y) {
		for (int x = 0; x < 256; x += 2) {
			int val = vramBase[offset++];
			setPixel2x2(x + 0, y, getColor((val >> 4) & 15));
			setPixel2x2(x + 1, y, getColor((val >> 0) & 15));
		}
	}
}

void VramBitMappedView::paintEvent(QPaintEvent* /*event*/)
{
	QRect srcRect(0, 0, 512, 2 * lines);
	QRect dstRect(0, 0, int(512.0f * zoomFactor), int(2.0f * float(lines) * zoomFactor));
	QPainter qp(this);
	//qp.drawImage(rect(),image,srcRect);
	qp.drawPixmap(dstRect, pixImage, srcRect);
}

void VramBitMappedView::refresh()
{
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::mouseMoveEvent(QMouseEvent* e)
{
	static const unsigned bytesPerLine[] = {
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
	static const unsigned pixelsPerByte[] = {
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

	int x = int(float(e->x()) / zoomFactor);
	int y = int(float(e->y()) / zoomFactor) / 2;

	// I see negative y-coords sometimes, so for safety clip the coords
	x = std::clamp(x, 0, 511);
	y = std::clamp(y, 0, 255);

	if ((screenMode != 6) && (screenMode != 7)) {
		x /= 2;
	}

	unsigned offset = bytesPerLine[screenMode] * y
	                + x / pixelsPerByte[screenMode];
	unsigned addr = offset + vramAddress;
	int val = (screenMode >= 7) ? vramBase[interleave(addr)]
	                            : vramBase[addr];

	int color = [&] {
		switch (screenMode) {
			case 5: case 7:
				return ((x & 1) ? val : (val >> 4)) & 15;
			case 6:
				return (val >> (2 * (3 - (x & 3)))) & 3;
			case 8: case 10: case 11: case 12:
				return val;
			default:
				return 0; // avoid warning
		}
	}();
	emit imageHovered(x, y, color, addr, val);
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
	borderColor = std::clamp(value, 0, 15);
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
	setFixedSize(int(512.0f * zoomFactor), int(float(lines) * 2.0f * zoomFactor));
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

void VramBitMappedView::setVramSource(const uint8_t* adr)
{
	vramBase = adr;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setPaletteSource(const uint8_t* adr)
{
	palette = adr;
	decodePallet();
	decode();
	update();
}
