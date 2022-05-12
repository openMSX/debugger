#include "PalettePatch.h"
#include <QPainter>
#include "MSXPalette.h"

PalettePatch::PalettePatch(QWidget* parent, int palNr)
    : QPushButton(parent), msxPalNr(palNr)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

void PalettePatch::setMSXPalette(MSXPalette* pal)
{
    if (myPal != nullptr) {
        disconnect(pal, SIGNAL(paletteChanged()),
                   this, SLOT(paletteChanged()));
    }
    myPal = pal;
    connect(pal, SIGNAL(paletteChanged()),
            this, SLOT(paletteChanged()));
    paletteChanged();
}

//old method for PaletteDialog
void PalettePatch::updatePaletteChanged(const uint8_t* pal)
{
    int r = (pal[2 * msxPalNr + 0] & 0xf0) >> 4;
    int b = (pal[2 * msxPalNr + 0] & 0x0f);
    int g = (pal[2 * msxPalNr + 1] & 0x0f);
    auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
    myColor = qRgb(scale(r), scale(g), scale(b));
    //setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    update();
    //printf("PalettePatch::updatePaletteChanged %i\n", msxPalNr);
}

//new method for PaletteView
void PalettePatch::paletteChanged()
{
    myColor = myPal->color(msxPalNr);
    update();
}

void PalettePatch::setHighlightTest(int colorNr)
{
    bool s = colorNr == msxPalNr;
    if (isSelected == s) return;
    isSelected = s;
    update();
}

void PalettePatch::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    if (isEnabled()) {
        painter.setPen(isSelected ? Qt::white : QColor(myColor));
        painter.setBrush(QBrush(myColor));
    } else {
        painter.setPen(Qt::black);
        int v = qGray(myColor);
        painter.setBrush(QBrush(QColor(v, v, v)));
    }
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}
