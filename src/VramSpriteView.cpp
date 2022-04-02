#include "VramSpriteView.h"
#include "VDPDataStore.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>
#include "Convert.h"
#include <cmath>


VramSpriteView::VramSpriteView(QWidget* parent, mode drawer, bool singleSprite)
    : QWidget(parent), image(512, 512, QImage::Format_RGB32), drawMode(drawer), isSingleSpriteDrawer(singleSprite)
{
    // sprite size = 8x8, 16x16, 32x32 or 40x8, 48x16, 64x32 if EC bit respected
    calculateSizeOfSprites();

    if (isSingleSpriteDrawer) {
        setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        zoomFactor = 4;
        nrOfSpritesToShow = 1;
        recalcSpriteLayout(sizeOfSpritesHorizontal * zoomFactor);
    } else {
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHeightForWidth(true);
        setSizePolicy(sizePolicy);
        switch (drawer) {
            case PatternMode:
                nrOfSpritesToShow = 256;
                nrOfSpritesHorizontal = 32;
                nrOfSpritesVertical = 8;
                break;
            case SpriteAttributeMode:
            case ColorMode:
                nrOfSpritesToShow = 32;
                nrOfSpritesHorizontal = 16;
                nrOfSpritesVertical = 2;
                break;
        }
        zoomFactor = 2;
        recalcSpriteLayout(512); // do as if we are 512 pixels wide, just to have some imageWidth/height etc etc values
    }

    // Mouse update events when mouse is moved over the image, Quibus likes this
    // better then my preferred click-on-the-image
    setMouseTracking(true);
}

int VramSpriteView::heightForWidth(int width) const
{
    // How many sprites do fit in this width?
    float x = int(width / (sizeOfSpritesHorizontal * zoomFactor)); // Does an int divided by an int gives an int that would be converted to float?
    int h = int(ceilf(float(nrOfSpritesToShow) / x)) * sizeOfSpritesVertical * zoomFactor;
    //printf("\nheightForWidth sizeOfSpritesHorizontal %d sizeOfSpritesVertical %d  zoomFactor %d \n", sizeOfSpritesHorizontal, sizeOfSpritesVertical, zoomFactor);
    //printf("heightForWidth w=%d => total=%d x=%f => h=%d \n", width, nr_of_sprites_to_show, x, h);
    return h;
}

QSize VramSpriteView::sizeHint() const
{
    return {imageWidth, imageHeight};
}

void VramSpriteView::calculateImageSize()
{
    recalcSpriteLayout(size().width());
    updateGeometry();
}

void VramSpriteView::setVramSource(const uint8_t* adr)
{
    if (vramBase == adr) return;
    vramBase = adr;
    decodePalette();
    decode();
}

void VramSpriteView::setPaletteSource(const uint8_t* adr, bool useVDP)
{
    useVDPpalette = useVDP;
    if (palette == adr) return;
    palette = adr;
    decodePalette();
    decode();
}

void VramSpriteView::mousePressEvent(QMouseEvent* e)
{
    int x = 0; // TODO is this correct? 'x' and 'y' always remain '0'.
    int y = 0;

    if (auto info = infoFromMouseEvent(e)) {
        QString text;
        switch (drawMode) {
            case PatternMode:
                text = patternInfo(info->character);
                currentSpriteboxSelected = info->spriteBox;
                emit characterClicked(info->character);
                refresh(); // to do the drawgrid with selection
                break;
            case SpriteAttributeMode:
            case ColorMode:
                if (info->character < 32) {
                    int pat = vramBase[attributeTableAddress + 4 * info->character + 2];
                    text = patternInfo(pat, info->spriteBox);
                    currentSpriteboxSelected = info->spriteBox;
                    emit spriteboxClicked(info->spriteBox);
                    refresh(); // to do the drawgrid with selection
                }
                break;
        }
        emit imageClicked(x, y, info->character, text);
    }
}

void VramSpriteView::mouseMoveEvent(QMouseEvent* e)
{
    int x = 0; // TODO is this correct? 'x' and 'y' always remain '0'.
    int y = 0;
    if (auto info = infoFromMouseEvent(e)) {
        emit imagePosition(x, y, info->character);
    }
}

std::optional<VramSpriteView::MouseEventInfo> VramSpriteView::infoFromMouseEvent(QMouseEvent* e)
{
    //printf(" VramSpriteView::infoFromMouseEvent(%i, %i)  \n", e->x(), e->y());
    if (!vramBase) return {};

    // I see negative y-coords sometimes, so for safety clip the coords to zero as lowest value
    int x = std::max(0, e->x());
    int y = std::max(0, e->y());
    // maybe clip to maximum to furthest spritepixel drawn?
    //x = std::max(0, std::min(e->x(), nrOfSpritesHorizontal * sizeOfSpritesHorizontal * zoomFactor - 1));
    //y = std::max(0, std::min(e->y(), nrOfSpritesVertical   * sizeOfSpritesVertical   * zoomFactor - 1));
    if (x >= imageWidth) return {};
    if (y >= imageHeight) return {};

    int spriteBox = nrOfSpritesHorizontal * int(y / (sizeOfSpritesVertical * zoomFactor)) + int(x / (sizeOfSpritesHorizontal * zoomFactor));
    if (spriteBox >= nrOfSpritesToShow) return {};

    int character = (size16x16 && drawMode == PatternMode) ? 4 * spriteBox : spriteBox;

    return MouseEventInfo{spriteBox, character};
}

void VramSpriteView::warningImage()
{
    QPainter qp(&image);
    qp.setPen(Qt::black);
    qp.drawText(16, 16, "No sprites in this screenmode!");
}

void VramSpriteView::setZoom(float zoom)
{
    zoomFactor = std::max(1, int(zoom));
    calculateImageSize();
}

void VramSpriteView::paintEvent(QPaintEvent* /*e*/)
{
    QRect srcRect(0, 0, imageWidth, imageHeight);
    QRect dstRect(0, 0, imageWidth, imageHeight);
    //QRect dstRect(0, 0, int(2 * imageWidth * zoomFactor), int(2 * imageHeight * zoomFactor));
    QPainter qp(this);
    //qp.drawImage(rect(), image, srcRect);

    //debug code
//    static bool fw = true;
//    qp.fillRect(0, 0, size().width(), size().height(), fw ? Qt::black : Qt::white);
//    fw = !fw;
    qp.drawPixmap(dstRect, pixImage, srcRect);
//    qp.setPen(Qt::black);
//    qp.drawText(QPoint(3, 30), QString("w=%1 h=%2 nr_total=%3").arg(imageWidth).arg(imageHeight).arg(nr_of_sprites_to_show));
//    qp.drawText(QPoint(3, 50), QString("sw=%1 sh=%2 nr_x=%3 nr_y=%4 => %5").
//                arg(sizeOfSpritesHorizontal).
//                arg(sizeOfSpritesVertical).
//                arg(nrOfSpritesHorizontal).
//                arg(nrOfSpritesVertical).
//                arg(nrOfSpritesVertical * nrOfSpritesHorizontal));
}

void VramSpriteView::decode()
{
    if (!vramBase) return;
    image.fill(Qt::lightGray);
    if (spriteMode == 0) {
        warningImage();
    } else {
        switch (drawMode) {
            case PatternMode:
                decodePgt();
                break;
            case SpriteAttributeMode:
                decodeSpat();
                break;
            case ColorMode:
                decodeCol();
                break;
            }
        //and now draw grid if any
        if (!isSingleSpriteDrawer && gridEnabled && spriteMode) {
            drawGrid();
        }
    }
    pixImage = pixImage.fromImage(image);
    update();
}

void VramSpriteView::drawGrid()
{
    QPainter qp(&image);
    qp.setPen(QColor(55, 55, 55, 128));

    int startPosXX = sizeOfSpritesHorizontal * zoomFactor - 1;
    int startPosYY = sizeOfSpritesVertical   * zoomFactor - 1;
    int gridHeight = nrOfSpritesVertical   * sizeOfSpritesVertical   * zoomFactor;
    int gridWidth  = nrOfSpritesHorizontal * sizeOfSpritesHorizontal * zoomFactor;

    //first draw upper and left line
    qp.drawLine(0, 0, gridWidth, 0);
    qp.drawLine(0, 0, 0, gridHeight);

    for (int yy = startPosYY; yy < gridHeight; yy += sizeOfSpritesVertical * zoomFactor) {
        qp.drawLine(0, yy, gridWidth, yy);
    }
    for (int xx = startPosXX; xx < gridWidth; xx += sizeOfSpritesHorizontal * zoomFactor) {
        qp.drawLine(xx, 0, xx, gridHeight);
    }

    if (currentSpriteboxSelected >= 0 && currentSpriteboxSelected < nrOfSpritesToShow) {
        int x = sizeOfSpritesHorizontal * zoomFactor *    (currentSpriteboxSelected % nrOfSpritesHorizontal);
        int y = sizeOfSpritesVertical   * zoomFactor * int(currentSpriteboxSelected / nrOfSpritesHorizontal);
        int w = sizeOfSpritesHorizontal * zoomFactor;
        int h = sizeOfSpritesVertical   * zoomFactor;
        if (x == 0) {
            w--;
        } else {
            x--;
        }
        if (y == 0) {
            h--;
        } else {
            y--;
        }
        qp.setPen(QColor(255, 0, 0, 192));
        qp.setBrush(Qt::NoBrush);
        qp.drawRect(x, y, w, h);
    }
}

void VramSpriteView::decodePalette()
{
    if (!palette) return;

    for (int i = 0; i < 16; ++i) {
        int r = (palette[2 * i + 0] & 0xf0) >> 4;
        int b = (palette[2 * i + 0] & 0x0f);
        int g = (palette[2 * i + 1] & 0x0f);

        auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
        msxPalette[i] = qRgb(scale(r), scale(g), scale(b));
    }
}

void VramSpriteView::decodePgt()
{
    image.fill(QColor(Qt::lightGray));
    //scope for painter of drawSpriteAt calling setSpritePixel will fail
    //QPainter::begin: A paint device can only be painted by one painter at a time.
    //{
    //QPainter qp(&image);
    //qp.setPen(Qt::black);
    //qp.drawText(32, 32, "decodePgt!");
    //}
    auto fg = QColor(Qt::white).rgb();
    auto bg = QColor(Qt::lightGray).rgb();
    if (isSingleSpriteDrawer) {
        drawMonochromeSpriteAt(charToDisplay, 0, 0, 0, fg, bg);
        if (size16x16) {
            drawMonochromeSpriteAt(charToDisplay + 1, 0, 0, 1, fg, bg);
            drawMonochromeSpriteAt(charToDisplay + 2, 0, 1, 0, fg, bg);
            drawMonochromeSpriteAt(charToDisplay + 3, 0, 1, 1, fg, bg);
        }
    } else {
        for (int i = 0; i < nrOfSpritesToShow; ++i) {
            int j = i * (size16x16 ? 4 : 1);
            drawMonochromeSpriteAt(j, i, 0, 0, fg, bg);
            if (size16x16) {
                drawMonochromeSpriteAt(j + 1, i, 0, 1, fg, bg);
                drawMonochromeSpriteAt(j + 2, i, 1, 0, fg, bg);
                drawMonochromeSpriteAt(j + 3, i, 1, 1, fg, bg);
            }
        }
    }
}

void VramSpriteView::decodeSpat()
{
    // Scope for painter of drawSpriteAt calling setSpritePixel will fail
    //QPainter::begin: A paint device can only be painted by one painter at a time.
    //{
    // QPainter qp(&image);
    // qp.setPen(Qt::black);
    // qp.drawText(232, 32, "decodespat!");
    //}
    if (spriteMode == 0) return;

    auto bgcolor = QColor(Qt::lightGray);
    for (int i = 0; i < 32; ++i) {
        drawSpatSprite(i, bgcolor);
    }
}

void VramSpriteView::decodeCol()
{
    if (spriteMode == 0) return;

    auto bgcolor = QColor(Qt::lightGray);
    for (int i = 0; i < 32; ++i) {
        drawColSprite(i, bgcolor);
    }
}

void VramSpriteView::drawColSprite(int entry, QColor& /*bgcolor*/)
{
    int color = vramBase[attributeTableAddress + 4 * entry + 3];
    int x =    (entry % nrOfSpritesHorizontal) * sizeOfSpritesHorizontal * zoomFactor;
    int y = int(entry / nrOfSpritesHorizontal) * sizeOfSpritesVertical   * zoomFactor;
    QPainter qp(&image);
    // Mode 0 already tested, we will not end up here with spritemode == 0
    int z = zoomFactor * ((useMagnification && useECbit) ? 2 : 1);
    if (spriteMode == 1) {
        bool ec = color & 128;
        QRgb fgColor = msxPalette[color & 15]; // drop EC and unused bits
        qp.fillRect(x, y, sizeOfSpritesHorizontal * z, sizeOfSpritesVertical * z, fgColor);
        qp.fillRect(x, y, z, sizeOfSpritesVertical * z, ec ? QRgb(Qt::white) : QRgb(Qt::black));
    } else if (spriteMode == 2) {
        for (int charrow = 0; charrow < (size16x16 ? 16 : 8); ++charrow) {
            uint8_t colorbyte = vramBase[colorTableAddress + 16 * entry + charrow];
            bool ec = colorbyte & 128;
            bool cc = colorbyte & 64;
            bool ic = colorbyte & 32;
            QRgb fgColor = msxPalette[colorbyte & 15]; // drop EC, CC and IC bits
            qp.fillRect(x,         y + charrow * z, sizeOfSpritesHorizontal * z, z, fgColor); // line with color
            qp.fillRect(x,         y + charrow * z,                           z, z, (ec ? QColor(Qt::white) : QColor(Qt::black))); // EC bit set -> white
            qp.fillRect(x +     z, y + charrow * z,                           z, z, (cc ? QColor(Qt::green) : QColor(Qt::black))); // CC bit set -> green
            qp.fillRect(x + 2 * z, y + charrow * z,                           z, z, (ic ? QColor(Qt::red)   : QColor(Qt::black))); // IC bit set -> blue

            if (z > 2) { // Nice black seperation line if possible
                qp.fillRect(x + 1 * z - 1, y + charrow * z, 1, z, QColor(Qt::black));
                qp.fillRect(x + 2 * z - 1, y + charrow * z, 1, z, QColor(Qt::black));
                qp.fillRect(x + 3 * z - 1, y + charrow * z, 1, z, QColor(Qt::black));
            }
        }
    } else {
        image.fill(QColor(Qt::darkCyan));
    }
}

void VramSpriteView::drawSpatSprite(int entry, QColor &bgColor)
{
    auto addr = attributeTableAddress + 4 * entry;
    int attrY = vramBase[addr + 0];
    //int attrX = vramBase[addr + 1];
    int pattern = vramBase[addr + 2];
    int color = vramBase[addr + 3];

    // Mode 0 already tested, we will not end up here with spritemode == 0
    if (spriteMode == 1) {
        bool ec = color & 128;
        QRgb fgColor = msxPalette[color & 15]; // drop EC and unused bits
        if (attrY == 208) { // in spritemode 1 if Y is equal to 208 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgColor = QColor(Qt::red);
        }
        if (size16x16) {
            pattern = pattern & 0xFC;
            drawMonochromeSpriteAt(pattern + 0, entry, 0, 0, fgColor, bgColor.rgb(), ec);
            drawMonochromeSpriteAt(pattern + 1, entry, 0, 1, fgColor, bgColor.rgb(), ec);
            drawMonochromeSpriteAt(pattern + 2, entry, 1, 0, fgColor, bgColor.rgb(), ec);
            drawMonochromeSpriteAt(pattern + 3, entry, 1, 1, fgColor, bgColor.rgb(), ec);
        } else {
            drawMonochromeSpriteAt(pattern, entry, 0, 0, fgColor, bgColor.rgb(), ec);
        }
    } else if (spriteMode == 2) {
        if (attrY == 216) { // in spritemode 2 if Y is equal to 216 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgColor = QColor(Qt::red);
        }
        if (size16x16) {
            pattern &= 0xFC;
            drawLineColoredSpriteAt(pattern + 0, entry, 0, 0, 0, bgColor.rgb());
            drawLineColoredSpriteAt(pattern + 1, entry, 0, 1, 8, bgColor.rgb());
            drawLineColoredSpriteAt(pattern + 2, entry, 1, 0, 0, bgColor.rgb());
            drawLineColoredSpriteAt(pattern + 3, entry, 1, 1, 8, bgColor.rgb());
        } else {
            drawLineColoredSpriteAt(pattern, entry, 0, 0, 0, bgColor.rgb());
        }
    }else {
        image.fill(QColor(Qt::darkCyan));
    }
}

void VramSpriteView::setSpritePixel(int x, int y, QRgb c)
{
    QPainter qp(&image);
    qp.fillRect(x * zoomFactor, y * zoomFactor, zoomFactor, zoomFactor, c);
    //image.setPixel();
}

QRgb VramSpriteView::getColor(int c)
{
    // TODO do we need to look at the TP bit???
    return msxPalette[c];
}

void VramSpriteView::setAttributeTableAddress(int value)
{
    attributeTableAddress = value;
    decode();
}

void VramSpriteView::setColorTableAddress(int value)
{
    colorTableAddress = value;
    decode();
}

QString VramSpriteView::colorInfo(uint8_t color) const
{
    bool ec = color & 128;
    int c = color & 15;

    QString colortext = QString("%1 (%2)").arg(int(color), 2)
                                          .arg(hexValue(color, 2));

    if (spriteMode == 1) {
        if (color > 15) {
            colortext.append(QString(" -> %1 %2").arg(ec ? "EC," : "")
                                                 .arg(c, 2));
        }
    } else {
        if (color > 15) {
            bool cc = color & 64;
            bool ic = color & 32;
            colortext.append(
                QString(" -> %1%2%3 %4").arg(ec ? "EC," : "")
                                        .arg(cc ? "CC," : "")
                                        .arg(ic ? "IC," : "")
                                        .arg(c, 2));
        }
    }
    return colortext;
}

QString VramSpriteView::patternInfo(int character, int spriteNr)
{
    // Mode 1 then gives
    QString info;
    if (spriteMode == 1 && spriteNr >= 0) {
        info.append(QString("Color data: %1\n").arg(colorInfo(vramBase[attributeTableAddress + 4 * spriteNr + 3])));
    }

    // Create header
    info.append("Pattern Data            ");
    if (spriteMode == 2 && spriteNr >= 0) {
        if (size16x16) {
            info.append("             ");
        }
        info.append("Color data");
    }
    info.append("\n");

    // Now build the text
    auto patBase = patternTableAddress + 8 * character;
    auto colBase = colorTableAddress + 16 * spriteNr;
    if (!size16x16) {
        // 8x8 character
        for (int charRow = 0; charRow < 8; ++charRow) {
            auto patAddr = patBase + charRow;
            auto colAddr = colBase + charRow;
            QString colorData;
            if (spriteMode == 2 && spriteNr >= 0) {
                colorData = QString("  %1: %2").arg(
                            hexValue(colAddr, 4),
                            colorInfo(vramBase[colAddr]));
            }
            info.append(QString("%1: %2 %3   %4\n").arg(
                            hexValue(patAddr, 4),
                            byteAsPattern(vramBase[patAddr]),
                            hexValue(vramBase[patAddr], 2),
                            colorData));
        }
    } else {
        for (int charRow = 0; charRow < 16; ++charRow) {
            auto patAddr = patBase + charRow;
            auto colAddr = colBase + charRow;
            QString colorData;
            if (spriteMode == 2 && spriteNr >= 0) {
                colorData = QString("  %1: %2").arg(
                            hexValue(colAddr, 4),
                            colorInfo(vramBase[colAddr]));
            }
            auto pat0 = vramBase[patAddr +  0];
            auto pat1 = vramBase[patAddr + 16];
            info.append(QString("%1: %2 %3 %4 %5   %6\n").arg(
                            hexValue(patAddr, 4),
                            byteAsPattern(pat0),
                            byteAsPattern(pat1),
                            hexValue(pat0, 2),
                            hexValue(pat1, 2),
                            colorData));
        }
    }
    return info;
}

QString VramSpriteView::byteAsPattern(uint8_t byte)
{
    QString val;
    for (int i = 7; i >= 0; --i) {
        val.append(QChar(byte & (1 << i) ? '1' : '.'));
    }
    return val;
}

void VramSpriteView::setPatternTableAddress(int value)
{
    patternTableAddress = value;
    decode();
}

void VramSpriteView::setDrawgrid(bool value)
{
    if (gridEnabled == value) return;

    gridEnabled = value;
    refresh();
}

void VramSpriteView::setUseMagnification(bool value)
{
    if (useMagnification == value) return;

    useMagnification = value;
    calculateSizeOfSprites();
    calculateImageSize();
    refresh();
}

void VramSpriteView::setCharToDisplay(int character)
{
    if (charToDisplay == character) return;

    charToDisplay = character;
    refresh();
}

void VramSpriteView::setSpriteboxClicked(int spBox)
{
    if (currentSpriteboxSelected == spBox) return;

    currentSpriteboxSelected = spBox;
    refresh();
}

void VramSpriteView::setCharacterClicked(int charBox)
{
    setSpriteboxClicked(charBox);
}

void VramSpriteView::recalcSpriteLayout(int width)
{
    nrOfSpritesHorizontal = std::max(1, width / (zoomFactor * sizeOfSpritesHorizontal)); // If to small to fit, then still set at least 1 otherwise we would be dividing by zero down the line!!
    nrOfSpritesVertical = int(ceilf(float(nrOfSpritesToShow) / float(nrOfSpritesHorizontal)));

    imageWidth  = nrOfSpritesHorizontal *sizeOfSpritesHorizontal * zoomFactor;
    imageHeight = nrOfSpritesVertical   * sizeOfSpritesVertical  * zoomFactor;
    image = QImage(imageWidth, imageHeight, QImage::Format_RGB32);
    refresh();
}

void VramSpriteView::resizeEvent(QResizeEvent *event)
{
    recalcSpriteLayout(event->size().width());
}

void VramSpriteView::refresh()
{
    // Reset pointers in case during boot these pointers weren't correctly set due to openMSX not having send over vram size...
    setVramSource(VDPDataStore::instance().getVramPointer());
    if (useVDPpalette) {
        setPaletteSource(VDPDataStore::instance().getPalettePointer(), true);
    }
    decodePalette();
    decode();
}

void VramSpriteView::setSpritemode(int value)
{
    spriteMode = value;
    refresh();
}

void VramSpriteView::setSize16x16(bool value)
{
    if (value == size16x16) return;

    size16x16 = value;

    if (isSingleSpriteDrawer) {
        calculateSizeOfSprites();
        imageWidth  = sizeOfSpritesHorizontal * zoomFactor;
        imageHeight = sizeOfSpritesVertical   * zoomFactor;
        image = QImage(imageWidth, imageHeight, QImage::Format_RGB32);
        setMaximumSize(imageWidth, imageHeight);
        setMinimumSize(imageWidth, imageHeight);
        updateGeometry();
    } else {
        if (drawMode == PatternMode) {
            nrOfSpritesToShow = size16x16 ? 64 : 256;
        }
        calculateSizeOfSprites();
        calculateImageSize();
    }
    refresh();
}

void VramSpriteView::setECinfluence(bool value)
{
    // EC bit is used when displaying Sprite attribute Table (and Sprite Color Table has same size for visual effect)
    // so make sure it remains false for the display of the Sprite Pattern Generator Table display
    bool newval = drawMode != PatternMode ? value : false;

    if (newval == useECbit) return;

    useECbit = newval;
    calculateSizeOfSprites();
    calculateImageSize();
    refresh();
}

void VramSpriteView::calculateSizeOfSprites()
{
    if (useECbit) {
        //in case of EC we need 32 pixels "to the left" extra.
        // but in that case also the magnification becomes important
        // when drawing a mode2 line shifted 32 pixels of a 16x16 sprite and magnification is off you get a 16 pixels gap. If magnified there is no gap
        sizeOfSpritesVertical = sizeOfSpritesHorizontal = (useMagnification ? 2 : 1) * (size16x16 ? 16 : 8);
        sizeOfSpritesHorizontal += 32;
    } else {
        sizeOfSpritesVertical = sizeOfSpritesHorizontal = size16x16 ? 16 : 8;
    }
}

void VramSpriteView::drawMonochromeSpriteAt(int character, int spritebox, int xoffset, int yoffset, QRgb fg, QRgb bg, bool ec)
{
    // x and y without zoomfactor, zoomfactor correction is done in setSpritePixel
    int x = (spritebox%nrOfSpritesHorizontal) * sizeOfSpritesHorizontal;
    int y = int(spritebox/nrOfSpritesHorizontal) * sizeOfSpritesVertical;

    int ec_offset = useECbit && !ec? 32 : 0; //draw more to the right if no ec but spritebox is extra wide to display ec effect

    QRgb backG = bg;
    bool doGrid = gridEnabled && 4 < (zoomFactor * ((useMagnification && useECbit) ? 2 : 1));
    for (int charRow = 0; charRow<8; ++charRow) {
        uint8_t patternByte = vramBase[patternTableAddress + 8 * character + charRow];
        for (int charCol = 0; charCol < 8; ++charCol) {
            uint8_t mask = 1 << (7 - charCol);
            if (doGrid) {
                backG = ((charRow + charCol) & 1) ? bg : QColor(bg).darker(110).rgb();
            }
            auto c = (patternByte & mask) ? fg : backG;
            if (useMagnification && useECbit) {
                auto xx = x + ec_offset + 2 * (8 * xoffset + charCol);
                auto yy = y             + 2 * (8 * yoffset + charRow);
                setSpritePixel(xx + 0, yy + 0, c);
                setSpritePixel(xx + 1, yy + 0, c);
                setSpritePixel(xx + 0, yy + 1, c);
                setSpritePixel(xx + 1, yy + 1, c);
            } else {
                setSpritePixel(x + 8 * xoffset + charCol + ec_offset, y + 8 * yoffset + charRow, c);
            }
        }
    }
}

void VramSpriteView::drawLineColoredSpriteAt(int character, int spriteBox, int xOffset, int yOffset, int rowOffset, QRgb bg)
{
    // x and y without zoomfactor, zoomfactor correction is done in setSpritePixel
    int x =    (spriteBox % nrOfSpritesHorizontal) * sizeOfSpritesHorizontal;
    int y = int(spriteBox / nrOfSpritesHorizontal) * sizeOfSpritesVertical;

    QRgb backG = bg;
    for (int charRow = 0; charRow < 8; ++charRow) {
        uint8_t patternByte = vramBase[patternTableAddress + 8 * character + charRow];
        uint8_t colorByte = vramBase[colorTableAddress + 16 * spriteBox + charRow + rowOffset];
        bool ec = colorByte & 128;
        int ec_offset = (useECbit && !ec) ? 32 : 0; // Draw more to the right if no ec but spritebox is extra wide to display ec effect

        QRgb fg = msxPalette[colorByte & 15]; // Drop EC, CC and IC bits
        for (int charCol = 0; charCol < 8; ++charCol) {
            uint8_t mask = 1 << (7 - charCol);
            if (gridEnabled && zoomFactor > 4) {
                backG = ((charRow + charCol) & 1) ? bg : QColor(bg).darker(110).rgb();
            }
            auto c = (patternByte & mask) ? fg : backG;
            if (useMagnification && useECbit) {
                auto xx = x + ec_offset + 2 * (8 * xOffset + charCol);
                auto yy = y             + 2 * (8 * yOffset + charRow);
                setSpritePixel(xx + 0, yy + 0, c);
                setSpritePixel(xx + 1, yy + 0, c);
                setSpritePixel(xx + 0, yy + 1, c);
                setSpritePixel(xx + 1, yy + 1, c);
            } else {
                setSpritePixel(x + 8 * xOffset + charCol + ec_offset, y + 8 * yOffset + charRow, c);
            }
        }
    }
}
