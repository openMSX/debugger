#ifndef VRAMSPRITEVIEW_H
#define VRAMSPRITEVIEW_H

#include <QWidget>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>
#include <cstdint>
#include <optional>

class VramSpriteView : public QWidget
{
    Q_OBJECT
public:
    enum mode
    {
        PatternMode,
        SpriteAttributeMode,
        ColorMode
    };

    explicit VramSpriteView(QWidget* parent = nullptr, mode drawer = PatternMode, bool singleSprite = false);

    [[nodiscard]] int heightForWidth(int width) const override;
    [[nodiscard]] QSize sizeHint() const override;

    void setVramSource(const uint8_t* adr);
    void setPaletteSource(const uint8_t* adr, bool useVDP);

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent (QMouseEvent* e) override;

    void warningImage();

    void setSpritemode(int value);
    void setPatternTableAddress(int value);
    void setAttributeTableAddress(int value);
    void setColorTableAddress(int value);

    QString patternInfo(int character, int spriteNr = -1);
    QString byteAsPattern(uint8_t byte);

    void calculateImageSize();
    void recalcSpriteLayout(int width);

public slots:
    void refresh();
    void setZoom(float zoom);
    void setDrawgrid(bool value);
    void setUseMagnification(bool value);
    void setSize16x16(bool value);
    void setECinfluence(bool value);

    void setCharToDisplay(int character);
    void setSpriteboxClicked(int spBox = -1); // for drawMode != PatternMode
    void setCharacterClicked(int charBox = -1); // when drawMode == PatternMode

signals:
    void imagePosition(int screenX, int screenY, int spriteNr);
    void imageClicked(int screenX, int screenY, int spriteNr, QString textInfo);

    // when clicked and selection made signals
    void spriteboxClicked(int spBox); // for drawMode != PatternMode
    void characterClicked(int charBox); // when drawMode == PatternMode

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void paintEvent(QPaintEvent* e) override;

    void decode();
    void decodePalette();
    void decodePgt();
    void decodeSpat();
    void decodeCol();

    void setSpritePixel(int x, int y, QRgb c);
    QRgb getColor(int c);

    QRgb msxPalette[16];
    QImage image;
    QPixmap pixImage;
    const uint8_t* palette = nullptr;
    const uint8_t* vramBase = nullptr;
    int patternTableAddress = 0;
    int attributeTableAddress = 0;
    int colorTableAddress = 0;

    int zoomFactor;
    bool gridEnabled = true;

    int spriteMode = 1; // 0 is no sprites, 1 = sprite mode 1 (msx1), 2 = sprite mode 2 (msx2)
    int imageWidth;
    int imageHeight;
    bool size16x16 = true;
    mode drawMode;
    int nrOfSpritesToShow;
    int nrOfSpritesHorizontal;
    int nrOfSpritesVertical;
    int sizeOfSpritesHorizontal;
    int sizeOfSpritesVertical;

    bool isSingleSpriteDrawer;
    bool useECbit = false;
    bool useVDPpalette = true;
    bool useMagnification = false; // only used when useECbit is true! if not this simply acts as zoomfactor...
    int charToDisplay = 0;
    int currentSpriteboxSelected = -1;

    void drawMonochromeSpriteAt(int character, int spriteBox, int xOffset, int yOffset, QRgb fg, QRgb bg, bool ec = false);
    void drawLineColoredSpriteAt(int character, int spriteBox, int xOffset, int yOffset, int rowOffset, QRgb bg);
    void drawGrid();
    void drawColSprite(int entry, QColor& bgColor);
    void drawSpatSprite(int entry, QColor& bgColor);

    struct MouseEventInfo {
        int spriteBox;
        int character;
    };
    std::optional<MouseEventInfo> infoFromMouseEvent(QMouseEvent* e);
    void calculateSizeOfSprites();
    [[nodiscard]] QString colorInfo(uint8_t color) const;
};

#endif // VRAMSPRITEVIEW_H
