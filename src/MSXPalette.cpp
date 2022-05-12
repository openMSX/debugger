#include "MSXPalette.h"
#include "CommClient.h"
#include <algorithm>

// default MSX palette
static const uint8_t defaultPalette[32] = {
//    RB  G
    0x00, 0,
    0x00, 0,
    0x11, 6,
    0x33, 7,
    0x17, 1,
    0x27, 3,
    0x51, 1,
    0x27, 6,
    0x71, 1,
    0x73, 3,
    0x61, 6,
    0x64, 6,
    0x11, 4,
    0x65, 2,
    0x55, 5,
    0x77, 7,
};

MSXPalette::MSXPalette(QObject* parent)
    : QObject{parent}
{
    memcpy(myPal, defaultPalette, sizeof(myPal));
    for (int i = 0; i < 16; ++i) {
        setColor(i, myPal[2 * i] >> 4, myPal[2 * i + 1], myPal[2 * i] & 15);
    }
}

void MSXPalette::copyDataFrom(const MSXPalette& source)
{
    memcpy(myPal, source.myPal, sizeof(myPal));
    memcpy(msxPalette, source.msxPalette, sizeof(msxPalette));
    emit paletteChanged();
}

void MSXPalette::setPalette(uint8_t* pal)
{
    int palchanged = memcmp(myPal, pal, sizeof(myPal));
    sourcePal = pal;
    if (palchanged != 0) {
        for (int i = 0; i < 16; ++i) {
            calculateColor(i, pal[2 * i] >> 4, pal[2 * i + 1], pal[2 * i] & 15);
        }
        emit paletteChanged();
    }
}

uint8_t* MSXPalette::getPalette()
{
    return myPal;
}

void MSXPalette::syncToSource()
{
    if (sourcePal == nullptr) {
        return;
    }
    memcpy(sourcePal, myPal, sizeof(myPal));

    if (syncToMSX) {
        syncToOpenMSX();
    }
    emit paletteSynced();
}

void MSXPalette::syncToSource(int i)
{
    if (sourcePal == nullptr) {
        return;
    }
    memcpy(sourcePal + 2 * i, myPal + 2 * i, 2);
    if (syncToMSX) {
        syncColorToOpenMSX(i);
    }
    emit paletteSynced();
}

void MSXPalette::setAutoSync(bool value)
{
    if (autoSync == value) {
        return;
    }
    autoSync = value;
    if (value) {
        syncToSource();
    }
}

void MSXPalette::setColor(int i, int r, int g, int b)
{
    calculateColor(i, r, g, b);
    emit paletteChanged();

    if (autoSync) {
        syncToSource(i);
    }
}

QRgb MSXPalette::color(int i)
{
    return msxPalette[std::clamp(i, 0, 15)];
}

void MSXPalette::calculateColor(int i, int r, int g, int b)
{
    i = std::clamp(i, 0, 15);
    r = std::clamp(r, 0, 7);
    g = std::clamp(g, 0, 7);
    b = std::clamp(b, 0, 7);

    myPal[2 * i + 0] = 16 * r + b;
    myPal[2 * i + 1] = g;

    auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
    msxPalette[i] = qRgb(scale(r), scale(g), scale(b));
}

void MSXPalette::syncToOpenMSX()
{
    for (int i = 0; i < 16; ++i) {
        syncColorToOpenMSX(i);
    }
}

void MSXPalette::syncColorToOpenMSX(int colorNr)
{
    CommClient::instance().sendCommand(
        new SimpleCommand(
            QString("setcolor %1 %2%3%4").arg(colorNr)
                                         .arg(myPal[2 * colorNr] >> 4)
                                         .arg(myPal[2 * colorNr + 1])
                                         .arg(myPal[2 * colorNr] & 15)));
}
