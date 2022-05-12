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

    void copyDataFrom(const MSXPalette& source);

    void setPalette(uint8_t* pal);
    void syncToSource();
    void syncToSource(int i);
    void setAutoSync(bool value);

    void setColor(unsigned i, unsigned r, unsigned g, unsigned b);
    QRgb color(unsigned i) const;

    bool syncToMSX = false; // TODO avoid public data members

signals:
    void paletteChanged();
    void paletteSynced();

private:
    void syncToOpenMSX();
    void syncColorToOpenMSX(int colorNr);

private:
    uint8_t* sourcePal = nullptr;
    uint8_t myPal[32];
    bool autoSync = false;
};

#endif // MSXPALETTE_H
