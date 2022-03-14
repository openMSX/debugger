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
    explicit VramSpriteView(QWidget* parent = nullptr, bool pgtdrawer = true, bool singlesprite = false);

    void setVramSource(const unsigned char* adr);
    void setPaletteSource(const unsigned char *adr);

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent (QMouseEvent* e) override;

    void warningImage();

    void setSize16x16(bool value);

    void setSpritemode(int value);


    void setPatternTableAddress(int value);

    void setAttributeTableAddress(int value);

    void setColorTableAddress(int value);

    QString patterninfo(int character, int spritenr = -1);

    QString byteAsPattern(unsigned char byte);

public slots:
    void refresh();
    void setZoom(float zoom);
    void setDrawgrid(bool value);

    void setCharToDisplay(int character);

signals:
    void imagePosition(int screenx, int screeny, int spritenr);
    void imageClicked(int screenx, int screeny, int spritenr, QString textinfo);

private:
    void paintEvent(QPaintEvent* e) override;

    void decode();
    void decodePallet();

    void decodepgt();
    void decodespat();

    void setPixel2x2(int x, int y, QRgb c);
    QRgb getColor(int c);

    QRgb msxpallet[16];
    QImage image;
    QPixmap piximage;
    const unsigned char* pallet;
    const unsigned char* vramBase;
    int patternTableAddress;
    int attributeTableAddress;
    int colorTableAddress;

    float zoomFactor;
    bool gridenabled;

    int spritemode;
    int imagewidth;
    int imageheight;
    bool size16x16;
    bool ispgtdrawer;
    bool isSingleSpriteDrawer;
    int charToDisplay;

    void drawCharAt(int character, int x, int y, QRgb fg, QRgb bg);
    void drawMode2CharAt(int character, int x, int y, int entry, int rowoffset, QRgb bg);
    void drawGrid();
    void drawSpatSprite(int entry,int screenx,int screeny,QColor &bgcolor);

    bool infoFromMouseEvent(QMouseEvent* e,int &x,int &y, int &character);
};

#endif // VRAMSPRITEVIEW_H
