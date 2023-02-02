#ifndef VRAMBITMAPPEDVIEW
#define VRAMBITMAPPEDVIEW

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>
#include <cstdint>

class MSXPalette;

class VramBitMappedView : public QWidget
{
	Q_OBJECT
public:
	VramBitMappedView(QWidget* parent = nullptr);

	void setZoom(float zoom);

	void setScreenMode(int mode);
	void setLines(int nrLines);
	void setVramSource(const uint8_t* adr);
	void setVramAddress(int adr);
    void setPaletteSource(MSXPalette* adr);
	void setBorderColor(int value);

	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent (QMouseEvent* e) override;

	void refresh();

signals:
	void imageChanged();
	void imageHovered(int xCoordMsx, int yCoordMsx, int color,
	                  unsigned addr, int byte);
	void imageClicked(int xCoordMsx, int yCoordMsx, int color,
	                  unsigned addr, int byte);

private:
	void paintEvent(QPaintEvent* e) override;

	void decode();
	void decodeSCR5();
	void decodeSCR6();
	void decodeSCR7();
	void decodeSCR8();
	void decodeSCR10();
	void decodeSCR12();
	void setPixel2x2(int x, int y, QRgb c);
	void setPixel1x2(int x, int y, QRgb c);
	QRgb getColor(int c);

private:
	QImage image;
	QPixmap pixImage;
    MSXPalette* palette = nullptr;
	const uint8_t* vramBase = nullptr;
	float zoomFactor;
	unsigned vramAddress = 0;
	int lines = 212;
	int screenMode = 5;
	int borderColor = 0;
};

#endif // VRAMBITMAPPEDVIEW
