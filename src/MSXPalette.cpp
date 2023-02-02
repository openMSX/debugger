#include "MSXPalette.h"
#include "CommClient.h"
#include <algorithm>
#include <cassert>

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
    emit paletteChanged();
}

void MSXPalette::copyDataFrom(const MSXPalette& source)
{
    memcpy(myPal, source.myPal, sizeof(myPal));
    emit paletteChanged();
}

void MSXPalette::setPalette(uint8_t* pal)
{
    sourcePal = pal;
    if (memcmp(myPal, pal, sizeof(myPal)) != 0) {
        memcpy(myPal, pal, sizeof(myPal));
        emit paletteChanged();
    }
}

void MSXPalette::syncToSource()
{
    if (!sourcePal) return;

    memcpy(sourcePal, myPal, sizeof(myPal));
    if (syncToMSX) {
        syncToOpenMSX();
    }
    emit paletteSynced();
}

void MSXPalette::syncToSource(int i)
{
    if (!sourcePal) return;

    memcpy(sourcePal + 2 * i, myPal + 2 * i, 2);
    if (syncToMSX) {
        syncColorToOpenMSX(i);
    }
    emit paletteSynced();
}

void MSXPalette::setAutoSync(bool value)
{
    if (autoSync == value) return;

    autoSync = value;
    if (value) {
        syncToSource();
    }
}

void MSXPalette::setColor(unsigned i, unsigned r, unsigned g, unsigned b)
{
    assert(i < 16);
    assert(r < 8);
    assert(g < 8);
    assert(b < 8);
    myPal[2 * i + 0] = (r << 4) | b;
    myPal[2 * i + 1] = g;

    emit paletteChanged();

    if (autoSync) {
        syncToSource(i);
    }
}

QRgb MSXPalette::color(unsigned i) const
{
    assert(i < 16);
    int r = (myPal[2 * i + 0] >> 4) & 7;
    int g = (myPal[2 * i + 1] >> 0) & 7;
    int b = (myPal[2 * i + 0] >> 0) & 7;
    auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
    return qRgb(scale(r), scale(g), scale(b));
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
