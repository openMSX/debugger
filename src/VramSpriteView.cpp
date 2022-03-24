#include "VramSpriteView.h"
#include "VDPDataStore.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>
#include "Convert.h"
#include <cmath>


VramSpriteView::VramSpriteView(QWidget *parent, mode drawer, bool singlesprite)
    : QWidget(parent), image(512, 512, QImage::Format_RGB32), drawMode(drawer),isSingleSpriteDrawer(singlesprite)
{
    pallet = nullptr;
    vramBase = nullptr;
    gridenabled = true;

    colorTableAddress=0;
    patternTableAddress=0;
    attributeTableAddress=0;

    size16x16=true;
    useECbit=false;
    useVDPpalette=true;
    useMagnification=false;
    currentSpriteboxSelected=-1;
    // sprite size=8x8,16x16,32x32 or 40x8,48x16,64x32 if EC bit respected
    calculate_size_of_sprites();


    charToDisplay=0;

    spritemode=1; //0 is no sprites, 1=sprite mode 1(msx1), 2=spritemode2 (msx2)
    if (isSingleSpriteDrawer){
        setSizePolicy( QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred) );
        zoomFactor=4;
        nr_of_sprites_to_show=1;
        recalcSpriteLayout(size_of_sprites_horizontal*zoomFactor);
    } else {
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred );
        sizePolicy.setHeightForWidth(true);
        setSizePolicy(sizePolicy);
        switch (drawer) {
        case PatternMode:
            nr_of_sprites_to_show=256;
            nr_of_sprites_horizontal=32;
            nr_of_sprites_vertical=8;
            break;
        case SpriteAttributeMode:
        case ColorMode:
            nr_of_sprites_to_show=32;
            nr_of_sprites_horizontal=16;
            nr_of_sprites_vertical=2;
            break;
        }
        zoomFactor=2;
        recalcSpriteLayout(512); // do as if we are 512 pixels wide, just to have some imagewidth/height etc etc values
    };


    // mouse update events when mouse is moved over the image, Quibus likes this
    // better then my preferd click-on-the-image
    setMouseTracking(true);
}

int VramSpriteView::heightForWidth(int width) const
{
    //how many sprites do fit in this width?
    float x=int(width/(size_of_sprites_horizontal*zoomFactor));  // does an int divided by an int gives an int that would be converted to float?
    int h=ceil(float(nr_of_sprites_to_show)/x)*size_of_sprites_vertical*zoomFactor;
//    printf("\nheightForWidth size_of_sprites_horizontal %d size_of_sprites_vertical %d  zoomFactor %d \n",size_of_sprites_horizontal,size_of_sprites_vertical,zoomFactor);
//    printf("heightForWidth w=%d => total=%d x=%f => h=%d \n",width,nr_of_sprites_to_show,x,h);

    return h;
}

QSize VramSpriteView::sizeHint() const
{
    return QSize(imagewidth,imageheight);
}

void VramSpriteView::calculateImageSize()
{
    recalcSpriteLayout(size().width());
    updateGeometry();
}

void VramSpriteView::setVramSource(const unsigned char *adr)
{
    if (vramBase == adr) return;
    vramBase = adr;
    decodePallet();
    decode();
}

void VramSpriteView::setPaletteSource(const unsigned char *adr, bool useVDP)
{
    useVDPpalette=useVDP;
    if (pallet == adr) return ;
    pallet = adr;
    decodePallet();
    decode();
}

void VramSpriteView::mousePressEvent(QMouseEvent *e)
{
    int x=0;
    int y=0;
    int character=0;
    int spritebox=0;

    if (infoFromMouseEvent(e,spritebox,character)){
        QString text;
        switch (drawMode) {
            case PatternMode:
                text=patterninfo(character);
                currentSpriteboxSelected=spritebox;
                emit characterClicked(character);
                refresh(); //to do the drawgrid with selection
            break;
        case SpriteAttributeMode:
        case ColorMode:
            if (character<32) {
                int pat=vramBase[attributeTableAddress+4*character+2];
                text=patterninfo(pat,spritebox);
                currentSpriteboxSelected=spritebox;
                emit spriteboxClicked(spritebox);
                refresh(); //to do the drawgrid with selection
            };
            break;
        }
        emit imageClicked(x,y,character,text);
    }
}

void VramSpriteView::mouseMoveEvent(QMouseEvent *e)
{
    int x=0;
    int y=0;
    int character=0;
    int spritebox=0;
    if (infoFromMouseEvent(e,spritebox,character)){
        emit imagePosition(x,y,character);
    };
}

bool VramSpriteView::infoFromMouseEvent(QMouseEvent *e, int &spritebox, int &character)
{
//    printf(" VramSpriteView::infoFromMouseEvent(%i,%i)  \n",e->x(),e->y());

    // I see negative y-coords sometimes, so for safety clip the coords to zero as lowest value
    int x = std::max(0, e->x());
    int y = std::max(0, e->y());
    // maybe clip to maximum to fartest spritepixel drawn?
    //x = std::max(0, std::min(e->x(),nr_of_sprites_horizontal*size_of_sprites_horizontal*zoomFactor-1 ));
    //y = std::max(0, std::min(e->y(),nr_of_sprites_vertical*size_of_sprites_vertical*zoomFactor-1 ));

    if (x>=imagewidth || y>=imageheight|| vramBase==nullptr){
        return false;
    };

    spritebox=character=nr_of_sprites_horizontal*int(y/(size_of_sprites_vertical*zoomFactor)) + int(x/(size_of_sprites_horizontal*zoomFactor));

    if (character>=nr_of_sprites_to_show){
        return false;
    };
    if (size16x16 && drawMode==PatternMode){
        character=character*4;
    }

    return true;
}


void VramSpriteView::warningImage()
{
    QPainter qp(&image);
    qp.setPen(Qt::black);
    qp.drawText(16,16,QString("No sprites in this screenmode!"));

}

void VramSpriteView::setZoom(float zoom)
{
    zoomFactor = std::max(1, int(zoom));
    calculateImageSize();

}

void VramSpriteView::paintEvent(QPaintEvent *e)
{
    QRect srcRect(0, 0, imagewidth, imageheight);
    QRect dstRect(0, 0, imagewidth, imageheight);
    //QRect dstRect(0, 0, int(2*imagewidth*zoomFactor), int(2*imageheight*zoomFactor));
    QPainter qp(this);
    //qp.drawImage(rect(),image,srcRect);

    //debug code
//    static bool fw=true;
//    qp.fillRect(0,0,size().width(),size().height(),fw?Qt::black:Qt::white);
//    fw=!fw;
    qp.drawPixmap(dstRect, piximage, srcRect);
//    qp.setPen(Qt::black);
//    qp.drawText(QPoint(3,30),QString("w=%1 h=%2 nr_total=%3").arg(imagewidth).arg(imageheight).arg(nr_of_sprites_to_show));
//    qp.drawText(QPoint(3,50),QString("sw=%1 sh=%2 nr_x=%3 nr_y=%4 => %5").
//                arg(size_of_sprites_horizontal).
//                arg(size_of_sprites_vertical).
//                arg(nr_of_sprites_horizontal).
//                arg(nr_of_sprites_vertical).
//                arg(nr_of_sprites_vertical*nr_of_sprites_horizontal)
//                );
}

void VramSpriteView::decode()
{
    if (!vramBase) return;
    image.fill(Qt::lightGray);
    if (spritemode==0){
        warningImage();
    } else {
        switch (drawMode) {
            case PatternMode:
                decodepgt();
                break;
            case SpriteAttributeMode:
                decodespat();
                break;
            case ColorMode:
                decodecol();
                break;
            }
        //and now draw grid if any
        if (!isSingleSpriteDrawer && gridenabled && spritemode){
            drawGrid();
        };
    };
    piximage = piximage.fromImage(image);
    update();
}

void VramSpriteView::drawGrid()
{
    QPainter qp(&image);
    qp.setPen(QColor(55,55,55,128));

    int startposxx=size_of_sprites_horizontal*zoomFactor-1;
    int startposyy=size_of_sprites_vertical*zoomFactor-1;
    int gridheight=nr_of_sprites_vertical*size_of_sprites_vertical*zoomFactor;
    int gridwidth=nr_of_sprites_horizontal*size_of_sprites_horizontal*zoomFactor;

    //first draw upper and left line
    qp.drawLine(0,0,  gridwidth,0);
    qp.drawLine(0,0,  0,gridheight);

    for (int yy=startposyy; yy<gridheight; yy+=size_of_sprites_vertical*zoomFactor ){
        qp.drawLine(0,yy,gridwidth,yy);
    }
    for (int xx=startposxx; xx<gridwidth; xx+=size_of_sprites_horizontal*zoomFactor){
        qp.drawLine(xx,0,xx,gridheight);
    }

    if (currentSpriteboxSelected>=0 && currentSpriteboxSelected<nr_of_sprites_to_show){
        int x=size_of_sprites_horizontal*zoomFactor*(currentSpriteboxSelected%nr_of_sprites_horizontal);
        int y=size_of_sprites_vertical*zoomFactor*int(currentSpriteboxSelected/nr_of_sprites_horizontal);
        int w=size_of_sprites_horizontal*zoomFactor;
        int h=size_of_sprites_vertical*zoomFactor;
        if (x==0){
            w--;
        } else {
            x--;
        };
        if (y==0){
            h--;
        } else {
            y--;
        };
        qp.setPen(QColor(255,0,0,192));
        qp.setBrush(Qt::NoBrush);
        qp.drawRect(x,y,w,h);

    }
}


void VramSpriteView::decodePallet()
{
    if (!pallet) return;

    for (int i = 0; i < 16; ++i) {
        int r = (pallet[2 * i + 0] & 0xf0) >> 4;
        int b = (pallet[2 * i + 0] & 0x0f);
        int g = (pallet[2 * i + 1] & 0x0f);

        r = (r >> 1) | (r << 2) | (r << 5);
        b = (b >> 1) | (b << 2) | (b << 5);
        g = (g >> 1) | (g << 2) | (g << 5);

        msxpallet[i] = qRgb(r, g, b);
    }
}

void VramSpriteView::decodepgt()
{
    image.fill(QColor(Qt::lightGray));
    //scope for painter of drawSpriteAt calling setSpritePixel will fail
    //QPainter::begin: A paint device can only be painted by one painter at a time.
    //{
    //QPainter qp(&image);
    //qp.setPen(Qt::black);
    //qp.drawText(32,32,QString("decodepgt!"));
    //};
    if (isSingleSpriteDrawer){
        drawMonochromeSpriteAt(charToDisplay,0, 0,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
        if (size16x16){
            drawMonochromeSpriteAt(charToDisplay+1,0, 0,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            drawMonochromeSpriteAt(charToDisplay+2,0, 1,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            drawMonochromeSpriteAt(charToDisplay+3,0, 1,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
        }
    } else {
        for(int i=0;i<nr_of_sprites_to_show;i++){
            int j=i*(size16x16?4:1);
            drawMonochromeSpriteAt(j,i, 0,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            if (size16x16){
                drawMonochromeSpriteAt(j+1,i, 0,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
                drawMonochromeSpriteAt(j+2,i, 1,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
                drawMonochromeSpriteAt(j+3,i, 1,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            }

        }
    }
}

void VramSpriteView::decodespat()
{
    //scope for painter of drawSpriteAt calling setSpritePixel will fail
    //QPainter::begin: A paint device can only be painted by one painter at a time.
    //{
    //QPainter qp(&image);
    //qp.setPen(Qt::black);
    //qp.drawText(232,32,QString("decodespat!"));
    //};
    if (spritemode==0){
        return;
    }

    QColor bgcolor=QColor(Qt::lightGray);
    for(int i=0;i<32;i++){
        drawSpatSprite(i,bgcolor);
    }
}

void VramSpriteView::decodecol()
{
    if (spritemode==0){
        return;
    }

    QColor bgcolor=QColor(Qt::lightGray);
    for(int i=0;i<32;i++){
        drawColSprite(i,bgcolor);
    }
}

void VramSpriteView::drawColSprite(int entry,QColor &bgcolor)
{
    int color=vramBase[attributeTableAddress+4*entry+3];
    int x=(entry%nr_of_sprites_horizontal) * size_of_sprites_horizontal*zoomFactor;
    int y=int(entry/nr_of_sprites_horizontal) * size_of_sprites_vertical*zoomFactor;
    QPainter qp(&image);
    //mode 0 already tested, we will not end up here with spritemode==0
    int z=zoomFactor*((useMagnification && useECbit)?2:1);
    if (spritemode==1){
        bool ec = color&128 ;
        QRgb fgcolor = msxpallet[color&15]; //drop EC and unused bits
        qp.fillRect(x,y,size_of_sprites_horizontal*z,size_of_sprites_vertical*z,fgcolor);
        qp.fillRect(x,y,z,size_of_sprites_vertical*z,
                    ec?QRgb(Qt::white):QRgb(Qt::black)
                    );
    } else if (spritemode==2){
        for (int charrow=0 ; charrow<(size16x16?16:8) ; charrow++){
            unsigned char colorbyte = vramBase[colorTableAddress+16*entry+charrow];
            bool ec=128&colorbyte;
            bool cc=64&colorbyte;
            bool ic=32&colorbyte;
            QRgb fgcolor=msxpallet[colorbyte&15]; //drop EC,CC and IC bits
            qp.fillRect(             x,   y+charrow*z,   size_of_sprites_horizontal*z,  z, fgcolor); // line with color
            qp.fillRect(             x,   y+charrow*z,                              z,  z, (ec?QColor(Qt::white):QColor(Qt::black)) ); // ec bit set -> white
            qp.fillRect(  x+z,   y+charrow*z,                              z,  z, (cc?QColor(Qt::green):QColor(Qt::black)) ); // cc bit set -> green
            qp.fillRect(x+2*z,   y+charrow*z,                              z,  z, (ic?QColor(Qt::red):QColor(Qt::black)) ); // ic bit set -> blue

            if (z>2){ // nice seperation black line if possible
                qp.fillRect(  x+z-1,   y+charrow*z,   1,  z, QColor(Qt::black));
                qp.fillRect(x+2*z-1,   y+charrow*z,   1,  z, QColor(Qt::black));
                qp.fillRect(x+3*z-1,   y+charrow*z,   1,  z, QColor(Qt::black));
            }
        };
    }else {
        image.fill(QColor(Qt::darkCyan));
    }

}

void VramSpriteView::drawSpatSprite(int entry,QColor &bgcolor)
{

    int atr_y=vramBase[attributeTableAddress+4*entry];
    //int atr_x=vramBase[attributeTableAddress+4*entry+1];
    int pattern=vramBase[attributeTableAddress+4*entry+2];
    int color=vramBase[attributeTableAddress+4*entry+3];

    //mode 0 already tested, we will not end up here with spritemode==0
    if (spritemode==1){
        bool ec = color&128 ;
        QRgb fgcolor = msxpallet[color&15]; //drop EC and unused bits
        if (atr_y==208){ //in spritemode 1 if Y is equal to 208 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgcolor=QColor(Qt::red);
        }
        if (size16x16){
            pattern = pattern & 0xFC;
            drawMonochromeSpriteAt(pattern,   entry, 0, 0, fgcolor,bgcolor.rgb(),ec);
            drawMonochromeSpriteAt(pattern+1, entry, 0, 1, fgcolor,bgcolor.rgb(),ec);
            drawMonochromeSpriteAt(pattern+2, entry, 1, 0, fgcolor,bgcolor.rgb(),ec);
            drawMonochromeSpriteAt(pattern+3, entry, 1, 1, fgcolor,bgcolor.rgb(),ec);
        } else {
            drawMonochromeSpriteAt(pattern, entry, 0,0, fgcolor,bgcolor.rgb(),ec);
        }
    } else if (spritemode==2){
        if (atr_y==216){ //in spritemode 2 if Y is equal to 216 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgcolor=QColor(Qt::red);
        }
        if (size16x16){
            pattern = pattern & 0xFC;
            drawLineColoredSpriteAt(pattern,   entry, 0, 0, 0, bgcolor.rgb());
            drawLineColoredSpriteAt(pattern+1, entry, 0, 1, 8, bgcolor.rgb());
            drawLineColoredSpriteAt(pattern+2, entry, 1, 0, 0, bgcolor.rgb());
            drawLineColoredSpriteAt(pattern+3, entry, 1, 1, 8, bgcolor.rgb());
        } else {
            drawLineColoredSpriteAt(pattern, entry, 0, 0, 0, bgcolor.rgb());
        }
    }else {
        image.fill(QColor(Qt::darkCyan));
    }


}


void VramSpriteView::setSpritePixel(int x, int y, QRgb c)
{
    QPainter qp(&image);
    qp.fillRect(x*zoomFactor,y*zoomFactor,zoomFactor,zoomFactor,c);
    //image.setPixel();
}

QRgb VramSpriteView::getColor(int c)
{
    // TODO do we need to look at the TP bit???
    return msxpallet[c];
}

void VramSpriteView::setAttributeTableAddress(int value)
{
    attributeTableAddress = value;
    decode();
}

void VramSpriteView::setColorTableAddress(int value)
{

    colorTableAddress=value;
    decode();
}

QString VramSpriteView::colorinfo(unsigned char color)
{
    bool ec=(color&128);
    int c=color&15;

    QString colortext=QString("%1 (%2)").arg(int(color),2)
                                        .arg(hexValue(color,2));

    if (spritemode==1){
        if (color>15){
            colortext.append(QString(" -> %1 %2").arg(ec?"EC,":"").arg(c,2));
        }
    } else {
        if (color>15){
            bool cc=color&64;
            bool ic=color&32;
            colortext.append(
                        QString(" -> %1%2%3 %4").arg(ec?"EC,":"")
                                                .arg(cc?"CC,":"")
                                                .arg(ic?"IC,":"")
                                                .arg(c,2)
                        );
        }
    }
    return colortext;
}

QString VramSpriteView::patterninfo(int character,int spritenr)
{
    QString info;

    //mode 1 then give
    if (spritemode==1 && spritenr>=0){
        info.append(QString("Color data: %1\n").arg(colorinfo(vramBase[attributeTableAddress+spritenr*4+3])));
    };

    //Create header
    info.append("Pattern Data            ");
    if (spritemode==2 && spritenr>=0){
        if (size16x16){
            info.append("             ");
        };
        info.append("Color data");
    };
    info.append("\n");

    QString colordata;

    //now build the text
    if (!size16x16){
        //8x8 character
        for (int charrow = 0 ; charrow < 8; charrow++) {
            int addr = patternTableAddress + 8*character + charrow;
            if (spritemode == 2 && spritenr>=0) {
                colordata = QString("  %1: %2").arg(
                            hexValue(colorTableAddress + spritenr*16 + charrow, 4),
                            colorinfo(vramBase[colorTableAddress + spritenr*16 + charrow])
                        );
            };
            info.append(QString("%1: %2 %3   %4\n").arg(
                            hexValue(addr, 4),
                            byteAsPattern(vramBase[patternTableAddress + character*8 +charrow]),
                            hexValue(vramBase[patternTableAddress + character*8 +charrow],2),
                            colordata
                        )
                    );
            colordata.clear();
        }
    } else {
        for (int charrow = 0 ; charrow < 15; charrow++) {
            int addr = patternTableAddress + 8*character + charrow;
            if (spritemode == 2 && spritenr>=0) {
                colordata = QString("  %1: %2").arg(
                            hexValue(colorTableAddress + spritenr*16 + charrow, 4),
                            colorinfo(vramBase[colorTableAddress + spritenr*16 + charrow])
                        );
            };
            info.append(QString("%1: %2 %3 %4 %5   %6\n").arg(
                            hexValue(addr, 4),
                            byteAsPattern(vramBase[patternTableAddress + character*8 +charrow]),
                            byteAsPattern(vramBase[patternTableAddress + character*8 +charrow + 16]),
                            hexValue(vramBase[patternTableAddress + character*8 +charrow],2),
                            hexValue(vramBase[patternTableAddress + character*8 +charrow + 16],2),
                            colordata
                        )
                    );
            colordata.clear();
        };
    };
    return info;



}

QString VramSpriteView::byteAsPattern(unsigned char byte)
{
    QString val;
    for (int i = 7; i >= 0 ; i--) {
        val.append(QChar(byte & (1 << i) ? '1' : '.'));
    };
    return val;
}


void VramSpriteView::setPatternTableAddress(int value)
{
    patternTableAddress = value;
    decode();
}

void VramSpriteView::setDrawgrid(bool value)
{
    if (gridenabled != value){
        gridenabled = value;
        refresh();
    };
}

void VramSpriteView::setUseMagnification(bool value)
{
    if (useMagnification==value){
        return;
    }
    useMagnification=value;
    calculate_size_of_sprites();
    calculateImageSize();
    refresh();
}

void VramSpriteView::setCharToDisplay(int character)
{
    if (charToDisplay != character){
        charToDisplay = character;
        refresh();
    };
}

void VramSpriteView::setSpriteboxClicked(int spbox)
{
    if (currentSpriteboxSelected==spbox){
        return;
    };

    currentSpriteboxSelected=spbox;
    refresh();
}

void VramSpriteView::setCharacterClicked(int charbox)
{
    setSpriteboxClicked(charbox);
}

void VramSpriteView::recalcSpriteLayout(int width)
{
    nr_of_sprites_horizontal=std::max(1,width/(zoomFactor*size_of_sprites_horizontal)); //if to small to fit, then still set at least 1 otherwise we would be dividing by zero down the line!!
    nr_of_sprites_vertical=ceil(float(nr_of_sprites_to_show)/float(nr_of_sprites_horizontal));

    imagewidth = nr_of_sprites_horizontal*size_of_sprites_horizontal*zoomFactor;
    imageheight = nr_of_sprites_vertical*size_of_sprites_vertical*zoomFactor;
    image=QImage(imagewidth,imageheight, QImage::Format_RGB32);
    refresh();
}
void VramSpriteView::resizeEvent(QResizeEvent *event)
{
    recalcSpriteLayout(event->size().width());
}

void VramSpriteView::refresh()
{
    //reget pointers in case during boot these pointers weren't correctly set due to openMSx not having send over vram size...
    setVramSource(VDPDataStore::instance().getVramPointer());
    if (useVDPpalette){
        setPaletteSource(VDPDataStore::instance().getPalettePointer(),true);
    };
    decodePallet();
    decode();
}

void VramSpriteView::setSpritemode(int value)
{
    spritemode = value;
    refresh();
}

void VramSpriteView::setSize16x16(bool value)
{
    if (value == size16x16){
        return;
    }

    size16x16 = value;

    if (isSingleSpriteDrawer){
        calculate_size_of_sprites();
        imagewidth = size_of_sprites_horizontal*zoomFactor;
        imageheight = size_of_sprites_vertical*zoomFactor;
        image=QImage(imagewidth,imageheight, QImage::Format_RGB32);
        setMaximumSize(imagewidth,imageheight);
        setMinimumSize(imagewidth,imageheight);
        updateGeometry();
    } else {
        if (drawMode==PatternMode){
            nr_of_sprites_to_show=size16x16?64:256;
        };
        calculate_size_of_sprites();
        calculateImageSize();
    }
    refresh();
}

void VramSpriteView::setECinfluence(bool value)
{
    // EC bit is used when displaying Sprite attribute Table (and Sprite Color Table has same size for visual effect)
    // so make sure it remains false for the display of the Sprite Pattern Generator Table display
    bool newval = drawMode!=PatternMode?value:false;

    if (newval==useECbit){
        return;
    };

    useECbit = newval;

    calculate_size_of_sprites();
    calculateImageSize();
    refresh();
}

void VramSpriteView::calculate_size_of_sprites()
{
    if (useECbit) {
        //in case of EC we need 32 pixels "to the left" extra.
        // but in that case also the magnification becomes important
        // when drawing a mode2 line shifted 32 pixels of a 16x16 sprite and magnification is off you get a 16 pixels gap. If magnified there is no gap
        size_of_sprites_vertical=size_of_sprites_horizontal=(useMagnification?2:1)*(size16x16?16:8);
        size_of_sprites_horizontal+=32;
    } else {
        size_of_sprites_vertical=size_of_sprites_horizontal=size16x16?16:8;
    }
}


void VramSpriteView::drawMonochromeSpriteAt(int character, int spritebox, int xoffset, int yoffset, QRgb fg, QRgb bg,bool ec)
{
    // x and y without zoomfactor, zoomfactor correction is done in setSpritePixel
    int x=(spritebox%nr_of_sprites_horizontal) * size_of_sprites_horizontal;
    int y=int(spritebox/nr_of_sprites_horizontal) * size_of_sprites_vertical;

    int ec_offset=useECbit && !ec? 32 : 0; //draw more to the right if no ec but spritebox is extra wide to display ec effect

    QRgb backg =bg ;
    bool dogrid = gridenabled && 4<(zoomFactor*((useMagnification && useECbit)?2:1)) ;
    for (int charrow=0 ; charrow<8 ; charrow++){
        unsigned char patternbyte = vramBase[patternTableAddress+8*character+charrow];
        for (int charcol=0 ; charcol<8 ; charcol++){
            unsigned char mask=1<<(7-charcol);
            if (dogrid){
                backg = 1&(charrow+charcol)?bg:QColor(bg).darker(110).rgb();
            };
            if (useMagnification && useECbit){
                setSpritePixel(x+ec_offset+2*(xoffset*8+charcol), y+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+1+2*(xoffset*8+charcol), y+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+2*(xoffset*8+charcol), y+1+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+1+2*(xoffset*8+charcol), y+1+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
            } else {
                setSpritePixel(x+xoffset*8+charcol+ec_offset, y+yoffset*8+charrow, (patternbyte&mask ? fg : backg ) );
            }
        }
    }

}

void VramSpriteView::drawLineColoredSpriteAt(int character,  int spritebox, int xoffset, int yoffset, int rowoffset, QRgb bg)
{
    // x and y without zoomfactor, zoomfactor correction is done in setSpritePixel
    int x=(spritebox%nr_of_sprites_horizontal) * size_of_sprites_horizontal;
    int y=int(spritebox/nr_of_sprites_horizontal) * size_of_sprites_vertical;

    QRgb backg =bg ;
    for (int charrow=0 ; charrow<8 ; charrow++){
        unsigned char patternbyte = vramBase[patternTableAddress+8*character+charrow];
        unsigned char colorbyte = vramBase[colorTableAddress+16*spritebox+charrow+rowoffset];
        bool ec=128&colorbyte;
        int ec_offset=useECbit && !ec? 32 : 0; //draw more to the right if no ec but spritebox is extra wide to display ec effect

        QRgb fg=msxpallet[colorbyte&15]; //drop EC,CC and IC bits
        for (int charcol=0 ; charcol<8 ; charcol++){
            unsigned char mask=1<<(7-charcol);
            if (gridenabled && zoomFactor>4){
                backg = 1&(charrow+charcol)?bg:QColor(bg).darker(110).rgb();
            };
            if (useMagnification && useECbit){
                setSpritePixel(x+ec_offset+2*(xoffset*8+charcol), y+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+1+2*(xoffset*8+charcol), y+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+2*(xoffset*8+charcol), y+1+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
                setSpritePixel(x+ec_offset+1+2*(xoffset*8+charcol), y+1+2*(yoffset*8+charrow), (patternbyte&mask ? fg : backg ) );
            } else {
                setSpritePixel(x+xoffset*8+charcol+ec_offset, y+yoffset*8+charrow, (patternbyte&mask ? fg : backg ) );
            }
        }
    }
}
