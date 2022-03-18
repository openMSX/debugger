#ifndef VRAMSPRITEVIEW_H
#define VRAMSPRITEVIEW_H

#include <QWidget>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QColor>

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

    explicit VramSpriteView(QWidget* parent = nullptr, mode drawer = PatternMode, bool singlesprite = false);

    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint() const;

    void setVramSource(const unsigned char* adr);
    void setPaletteSource(const unsigned char *adr);

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent (QMouseEvent* e) override;

    void warningImage();


    void setSpritemode(int value);


    void setPatternTableAddress(int value);

    void setAttributeTableAddress(int value);

    void setColorTableAddress(int value);

    QString patterninfo(int character, int spritenr = -1);

    QString byteAsPattern(unsigned char byte);

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
    void setSpriteboxClicked(int spbox); //for drawMode != PatternMode
    void setCharacterClicked(int charbox); //when drawMode == PatternMode

signals:
    void imagePosition(int screenx, int screeny, int spritenr);
    void imageClicked(int screenx, int screeny, int spritenr, QString textinfo);

    //when clicked and selection made signals
    void spriteboxClicked(int spbox); //for drawMode != PatternMode
    void characterClicked(int charbox); //when drawMode == PatternMode

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    void paintEvent(QPaintEvent* e) override;

    void decode();
    void decodePallet();

    void decodepgt();
    void decodespat();
    void decodecol();

    void setSpritePixel(int x, int y, QRgb c);
    QRgb getColor(int c);

    QRgb msxpallet[16];
    QImage image;
    QPixmap piximage;
    const unsigned char* pallet;
    const unsigned char* vramBase;
    int patternTableAddress;
    int attributeTableAddress;
    int colorTableAddress;

    int zoomFactor;
    bool gridenabled;

    int spritemode;
    int imagewidth;
    int imageheight;
    bool size16x16;
    mode drawMode;
    int nr_of_sprites_to_show;
    int nr_of_sprites_horizontal;
    int nr_of_sprites_vertical;
    int size_of_sprites_horizontal;
    int size_of_sprites_vertical;

    bool isSingleSpriteDrawer;
    bool useECbit;
    bool useMagnification; //only used when useECbit is true! if not this simply acts as zoomfactor...
    int charToDisplay;
    int currentSpriteboxSelected;

    void drawMonochromeSpriteAt(int character, int spritebox, int xoffset, int yoffset, QRgb fg, QRgb bg, bool ec=false);
    void drawLineColoredSpriteAt(int character, int spritebox, int xoffset, int yoffset, int rowoffset, QRgb bg);
    void drawGrid();
    void drawColSprite(int entry, QColor &bgcolor);
    void drawSpatSprite(int entry, QColor &bgcolor);

    bool infoFromMouseEvent(QMouseEvent* e, int &spritebox, int &character);
    void calculate_size_of_sprites();
    QString colorinfo(unsigned char color);
};

#endif // VRAMSPRITEVIEW_H
