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
        disconnect(myPal, &MSXPalette::paletteChanged, this, &PalettePatch::paletteChanged);
    }
    myPal = pal;
    connect(myPal, &MSXPalette::paletteChanged, this, &PalettePatch::paletteChanged);
    paletteChanged();
}

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
