#ifndef MSXPALETTE_H
#define MSXPALETTE_H

#include <QObject>
#include <QRgb>
#include <cstdint>

class MSXPalette : public QObject
{
    Q_OBJECT
public:
    explicit MSXPalette(QObject* parent = nullptr);
    MSXPalette& operator=(const MSXPalette& source);

    void setPalette(uint8_t* pal);
    uint8_t* getPalette();
    void syncToSource();
    void syncToSource(int i);
    void setAutoSync(bool value);

    void setColor(int i, int r, int g, int b);
    QRgb color(int i);

    bool syncToMSX = false; // TODO avoid public data members

signals:
    void paletteChanged();
    void paletteSynced();

private:
    void calculateColor(int i, int r, int g, int b);
    void syncToOpenMSX();
    void syncColorToOpenMSX(int colorNr);

private:
    uint8_t* sourcePal = nullptr;
    uint8_t myPal[32];
    QRgb msxPalette[16];
    bool autoSync = false;
};

#endif // MSXPALETTE_H
