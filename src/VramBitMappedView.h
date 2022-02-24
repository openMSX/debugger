#ifndef VRAMBITMAPPEDVIEW
#define VRAMBITMAPPEDVIEW

#include <QString>
#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>

class VramBitMappedView : public QWidget
{
	Q_OBJECT
public:
	VramBitMappedView(QWidget* parent = nullptr);

	void setZoom(float zoom);

	void setScreenMode(int mode);
	void setLines(int nrLines);
	void setVramSource(const unsigned char* adr);
	void setVramAddress(int adr);
	void setPaletteSource(const unsigned char* adr);
	void setBorderColor(int value);

	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent (QMouseEvent* e) override;

public slots:
	void refresh();

signals:
	void imageChanged();
	void imageHovered(int xcoormsx, int ycoormsx, int color,
	                  unsigned addr, int byte);
	void imageClicked(int xcoormsx, int ycoormsx, int color,
	                  unsigned addr, int byte);

private:
	void paintEvent(QPaintEvent* e) override;

	void decode();
	void decodePallet();
	void decodeSCR5();
	void decodeSCR6();
	void decodeSCR7();
	void decodeSCR8();
	void decodeSCR10();
	void decodeSCR12();
	void setPixel2x2(int x, int y, QRgb c);
	void setPixel1x2(int x, int y, QRgb c);
	QRgb getColor(int c);

	QRgb msxpallet[16];
	QImage image;
	QPixmap piximage;
	const unsigned char* pallet;
	const unsigned char* vramBase;
	float zoomFactor;
	unsigned vramAddress;
	int lines;
	int screenMode;
	int borderColor;
};

#endif // VRAMBITMAPPEDVIEW
