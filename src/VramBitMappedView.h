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
    VramBitMappedView(QWidget *parent = 0);
    void load(QString filename);
    void setZoom(float zoom);

    void setScreenMode(int mode);
    void setLines(int nrLines);
    void setVramSource(unsigned char* adr);
    void setVramAddress(int adr);
    void setPaletteSource(unsigned char* adr);
    void setBorderColor(int value);

    QRgb msxpallet[16];

    void mousePressEvent ( QMouseEvent * e );
    void mouseMoveEvent ( QMouseEvent * e );

public slots:
    void refresh();

signals:
    void imageChanged();
    void imagePosition(int xcoormsx,int ycoormsx, int color, unsigned addr, int byte);
    void imageClicked(int xcoormsx,int ycoormsx, int color, unsigned addr, int byte);

protected:
    void paintEvent( QPaintEvent* );

    void decode();
    void decodePallet();
    void decodeSCR5();
    void decodeSCR6();
    void decodeSCR7();
    void decodeSCR8();
    void decodeSCR10();
    void decodeSCR11();
    void decodeSCR12();

private:
    QImage image;
    QPixmap piximage;
    float zoomFactor;
    unsigned char* pallet;
    unsigned char* vramBase;
    unsigned int vramAddress;
    int lines;
    int screenMode;
    int borderColor;

};

#endif // VRAMBITMAPPEDVIEW

