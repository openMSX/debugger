#ifndef VRAMBITMAPPEDVIEW
#define VRAMBITMAPPEDVIEW

#include <QString>
#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>
#include <cstdint>
#include <optional>

class VramTiledView : public QWidget
{
	Q_OBJECT
public:
	VramTiledView(QWidget* parent = nullptr);

	void setZoom(float zoom);
	void setScreenMode(int mode);
	void setLines(int nrLines);
	void setVramSource(const uint8_t* adr);
    void setNameTableAddress(int adr);
    void setPatternTableAddress(int adr);
    void setColorTableAddress(int adr);
    void setPaletteSource(const uint8_t *adr);
	void setBorderColor(int value);
    void setDrawGrid(bool value);
    void setTpBit(bool value);
    void setTabletoShow(int value);
    void setUseBlink(bool value);
    void setForcedScreenRows(float value);
    void setHighlightChar(int value);

    [[nodiscard]] unsigned getNameTableAddress() const;
    [[nodiscard]] unsigned getPatternTableAddress() const;
    [[nodiscard]] unsigned getColorTableAddress() const;
    [[nodiscard]] int getScreenMode() const;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent (QMouseEvent* e) override;

    void warningImage();

    // Used to draw a character on the external image used when clicked on the VramTiledView widget
    void drawCharAtImage(int character, int x, int y, QImage& image); // Draw 8x8 character on reference image at topleft
    QString textInfo(int x, int y, int character);

    [[nodiscard]] const uint8_t* getPaletteSource() const;

    void refresh();

signals:
    void imageHovered(int screenX, int screenY, int character);
    void imageClicked(int screenX, int screenY, int character, QString textinfo);
    void highlightCount(uint8_t character, int count);

private:
	void paintEvent(QPaintEvent* e) override;

	void decode();
	void decodePalette();
    void decodePatternTable();
    void decodeNameTable();
    void overLayNameTable();
    void setPixel1x2(int x, int y, QRgb c);
    void setPixel2x2(int x, int y, QRgb c);
	QRgb getColor(int c);

    void decodePatternTableRegularChars();
    void decodePatternTableMultiColor();
    void decodeNameTableRegularChars();
    void decodeNameTableMultiColor();
    uint8_t getCharColorByte(int character, int x, int y, int row);
    void drawCharAt(int character, int x, int y); // Draw 8x8 character on given location in image

    struct MouseEventInfo {
        int x, y;
        int character;
    };
    std::optional<MouseEventInfo> infoFromMouseEvent(QMouseEvent* e);

    QString byteAsPattern(uint8_t byte);

private:
	QRgb msxPalette[16];
	QImage image;
	QPixmap pixImage;
    const uint8_t* palette = nullptr;
	const uint8_t* vramBase = nullptr;
	float zoomFactor;
    bool drawGrid = true;

    int patternTableAddress = 0;
    int nameTableAddress = 0;
    int colorTableAddress = 0;

    int screenWidth = 32;
    int screenHeight = 24;
    int charWidth = 8;
	int horiStep = 16;

	int lines = 212;
	int screenMode = 0;
    int tableToShow = 0;
	int borderColor = 0;
    float forcedScreenRows = 0;
    int highlightChar = -1; // anything outside 0-255 will do
    bool useBlink = false;
    bool tpBit = false;
};

#endif // VRAMBITMAPPEDVIEW
