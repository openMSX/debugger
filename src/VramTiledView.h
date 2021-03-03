#ifndef VRAMBITMAPPEDVIEW
#define VRAMBITMAPPEDVIEW

#include <QString>
#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>

class VramTiledView : public QWidget
{
	Q_OBJECT
public:
	VramTiledView(QWidget* parent = nullptr);

	void setZoom(float zoom);
	void setScreenMode(int mode);
	void setLines(int nrLines);
	void setVramSource(const unsigned char* adr);
    void setNameTableAddress(int adr);
    void setPatternTableAddress(int adr);
    void setColorTableAddress(int adr);
	void setPaletteSource(const unsigned char* adr);
	void setBorderColor(int value);
    void setDrawgrid(bool value);
    void setTPbit(bool value);
    void setTabletoShow(int value);
    void setUseBlink(bool value);
    void setForcedscreenrows(float value);
    void setHighlightchar(int value);

    unsigned getNameTableAddress() const;
    unsigned getPatternTableAddress() const;
    unsigned getColorTableAddress() const;
    int getScreenMode() const;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent (QMouseEvent* e) override;

    void warningImage();

    //used to draw a character on the external image used when clicked on the VramTiledView widget
    void drawCharAtImage(int character,int x, int y, QImage &image); //draw 8x8 character on reference image at topleft
    QString textinfo(int &x, int &y, int &character);


public slots:
    void refresh();

signals:
    void imagePosition(int screenx, int screeny, int character);
    void imageClicked(int screenx, int screeny, int character, QString textinfo);
    void highlightCount(unsigned char character, int count);

private:
	void paintEvent(QPaintEvent* e) override;

	void decode();
	void decodePallet();
    void decodePatternTable();
    void decodeNameTable();
    void overLayNameTable();
    void setPixel1x2(int x, int y, QRgb c);
    void setPixel2x2(int x, int y, QRgb c);
	QRgb getColor(int c);

	QRgb msxpallet[16];
	QImage image;
	QPixmap piximage;
	const unsigned char* pallet;
	const unsigned char* vramBase;
	float zoomFactor;
    bool drawgrid;

    int PatternTableAddress;
    int NameTableAddress;
    int ColorTableAddress;

    int screenwidth=32;
    int screenheight=24;
    int charwidth=8;
	int horistep=16;

	int lines;
	int screenMode;
    int tableToShow;
	int borderColor;
    float forcedscreenrows;
    int highlightchar;
    bool useBlink;
    bool TPbit;

    void decodePatternTableRegularChars();
    void decodePatternTableMultiColor();
    void decodeNameTableRegularChars();
    void decodeNameTableMultiColor();
    unsigned char getCharColorByte(int character, int x, int y, int row);
    void drawCharAt(int character,int x, int y); //draw 8x8 character on given location in image
    bool infoFromMouseEvent(QMouseEvent *e, int &x, int &y, int &character);

    QString byteAsPattern(unsigned char byte);
};

#endif // VRAMBITMAPPEDVIEW
