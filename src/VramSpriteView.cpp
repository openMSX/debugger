#include "VramSpriteView.h"
#include "VDPDataStore.h"
#include <QPainter>
#include <algorithm>
#include <cstdio>
#include "Convert.h"


VramSpriteView::VramSpriteView(QWidget *parent, bool pgtdrawer, bool singlesprite)
    : QWidget(parent), image(512, 512, QImage::Format_RGB32), ispgtdrawer(pgtdrawer),isSingleSpriteDrawer(singlesprite)
{
    pallet = nullptr;
    vramBase = nullptr;
    gridenabled = true;

    colorTableAddress=0;
    patternTableAddress=0;
    attributeTableAddress=0;

    charToDisplay=0;

    spritemode=1; //0 is no sprites, 1=sprite mode 1(msx1), 2=spritemode2 (msx2)
    if (isSingleSpriteDrawer){
        imagewidth=imageheight=16;
        setZoom(2.0f);
    } else {
        imagewidth=256;
        imageheight=pgtdrawer?64:256;
        setZoom(1.0f);
    };
    size16x16=false;

    // mouse update events when mouse is moved over the image, Quibus likes this
    // better then my preferd click-on-the-image
    setMouseTracking(true);
}

void VramSpriteView::setVramSource(const unsigned char *adr)
{
    vramBase = adr;
    decodePallet();
    decode();
}

void VramSpriteView::setPaletteSource(const unsigned char *adr)
{
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

    if (infoFromMouseEvent(e,x,y,character)){
        QString text;
        if (ispgtdrawer){
            text=patterninfo(character);
        } else if (character<32) {
            int pat=vramBase[attributeTableAddress+4*character+2];
            text=patterninfo(pat,character);
        };

        emit imageClicked(x,y,character,text);
    };
}

void VramSpriteView::mouseMoveEvent(QMouseEvent *e)
{
    int x=0;
    int y=0;
    int character=0;
    if (infoFromMouseEvent(e,x,y,character)){
        emit imagePosition(x,y,character);
    };
}

bool VramSpriteView::infoFromMouseEvent(QMouseEvent *e, int &x, int &y, int &character)
{
    x = int(e->x() / zoomFactor) / 2;
    y = int(e->y() / zoomFactor) / 2;
//    printf(" VramSpriteView::infoFromMouseEvent(%i,%i) => %i % i \n",e->x(),e->y(),x,y);

    // I see negative y-coords sometimes, so for safety clip the coords
    x = std::max(0, std::min(511, x));
    y = std::max(0, std::min(255, y));

    if (x>=imagewidth || y>=imageheight|| vramBase==nullptr){
        return false;
    };

    if (ispgtdrawer){
        character=(size16x16?
                       (((x&0xfff0)>>2)+((y&0xfff0)<<2)) :
                       ( (x>>3)+((y&0xfff8)<<2))
                   );
    } else {
        character=(((x&0xfff0)>>2)+((y&0xfff0)<<2)) / 4;
    };
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
    zoomFactor = std::max(1.0f, zoom);
//    if (vramBase!=nullptr){
//        const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
//    }
    setFixedSize(int(imagewidth * 2* zoomFactor), int(imageheight* 2 * zoomFactor));
    refresh();
}

void VramSpriteView::paintEvent(QPaintEvent *e)
{
    QRect srcRect(0, 0, 2*imagewidth, 2 * imageheight);
    QRect dstRect(0, 0, int(2*imagewidth*zoomFactor), int(2*imageheight*zoomFactor));
    QPainter qp(this);
    //qp.drawImage(rect(),image,srcRect);
    qp.drawPixmap(dstRect, piximage, srcRect);
}

void VramSpriteView::decode()
{
    if (!vramBase) return;
    image.fill(Qt::gray);
    if (spritemode==0){
        warningImage();
    } else {
        if (ispgtdrawer) {
            decodepgt();
        } else {
            decodespat();
        };
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
    int startpos=size16x16?15:7;
    int stephori=size16x16?16:8;
//    if (spritemode==2){
//        stephori*=2;
//    };
    int stepverti=size16x16?16:8;
    int gridheight=ispgtdrawer?imageheight:256;
    int gridwidth=ispgtdrawer?imagewidth:256;

    for (int yy=startpos; yy<gridheight; yy+=stepverti ){
        qp.drawLine(0,yy*2+1,gridwidth*2,yy*2+1);
    }
    for (int xx=startpos; xx<gridwidth; xx+=stephori){
        qp.drawLine(xx*2+1,0,xx*2+1,2*gridheight);
    }

}


void VramSpriteView::decodePallet()
{
    if (!pallet) return;
    printf("VramSpriteView::decodePallet  palletpointer %p \n",pallet);

    for (int i = 0; i < 16; ++i) {
        int r = (pallet[2 * i + 0] & 0xf0) >> 4;
        int b = (pallet[2 * i + 0] & 0x0f);
        int g = (pallet[2 * i + 1] & 0x0f);

        r = (r >> 1) | (r << 2) | (r << 5);
        b = (b >> 1) | (b << 2) | (b << 5);
        g = (g >> 1) | (g << 2) | (g << 5);

        msxpallet[i] = qRgb(r, g, b);
        printf("VramSpriteView::decodePallet  color %d => r %d  g %d  b %d \n",i,r,g,b);
    }
}

void VramSpriteView::decodepgt()
{
    printf("void VramSpriteView::decodepgt()\n");
    image.fill(QColor(Qt::lightGray));
    QPainter qp(&image);
    qp.setPen(Qt::black);
    qp.drawText(32,32,QString("decodepgt!"));
    if (isSingleSpriteDrawer){
        drawCharAt(charToDisplay,0,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
        if (size16x16){
            drawCharAt(charToDisplay+1,0,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            drawCharAt(charToDisplay+2,1,0, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            drawCharAt(charToDisplay+3,1,1, QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
        }
    } else {
        for(int i=0;i<256;i++){
            int j=size16x16?((i&0xc0) +((i&0x3E)>>1) + ((i&0x01)<<5)  ):(i);
//            drawCharAt(i,j&15,j>>4,QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
            drawCharAt(i,j&31,j>>5,QColor(Qt::white).rgb(),QColor(Qt::lightGray).rgb());
        }
    }
}

void VramSpriteView::decodespat()
{
    printf("void VramSpriteView::decodepgt()\n");
    QPainter qp(&image);
    qp.setPen(Qt::black);
    qp.drawText(32,32,QString("decodespat!"));
    if (spritemode==0){
        return;
    }

    QColor bgcolor=QColor(Qt::lightGray);
    for(int i=0;i<32;i++){
        int x=(i*2)&31;
        int y=(i>>4)*2;
        drawSpatSprite(i,x,y,bgcolor);
    }
}

void VramSpriteView::drawSpatSprite(int entry,int screenx,int screeny,QColor &bgcolor)
{

    int y=vramBase[attributeTableAddress+4*entry];
    int x=vramBase[attributeTableAddress+4*entry+1];
    int pattern=vramBase[attributeTableAddress+4*entry+2];
    int color=vramBase[attributeTableAddress+4*entry+3];
    printf("%d: %04x y %d     x %d      pattern %d  color  %d \n",entry, attributeTableAddress,y,x,pattern,color);

    //mode 0 already tested, we will not end up here with spritemode==0
    if (spritemode==1){
        QRgb fgcolor = msxpallet[color&15]; //drop EC and unused bits
        if (y==208){ //in spritemode 1 if Y is equal to 208 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgcolor=QColor(Qt::red);
        }
        if (size16x16){
            pattern = pattern & 0xFC;
            drawCharAt(pattern,   screenx  , screeny  , fgcolor,bgcolor.rgb());
            printf("     pattern %d   x %d y %d    %08x   %08x \n",pattern,   screenx  , screeny  , fgcolor,bgcolor.rgb());
            drawCharAt(pattern+1, screenx+0, screeny+1, fgcolor,bgcolor.rgb());
            printf("     pattern %d   x %d y %d    %08x   %08x \n",pattern+1, screenx+0, screeny+1, fgcolor,bgcolor.rgb());
            drawCharAt(pattern+2, screenx+1, screeny+0, fgcolor,bgcolor.rgb());
            printf("     pattern %d   x %d y %d    %08x   %08x \n",pattern+2, screenx+1, screeny+0, fgcolor,bgcolor.rgb());
            drawCharAt(pattern+3, screenx+1, screeny+1, fgcolor,bgcolor.rgb());
            printf("     pattern %d   x %d y %d    %08x   %08x \n",pattern+3, screenx+1, screeny+1, fgcolor,bgcolor.rgb());
        } else {
            drawCharAt(pattern,screenx,screeny,fgcolor,bgcolor.rgb());
        }
    } else if (spritemode==2){
        if (y==216){ //in spritemode 1 if Y is equal to 208 this and all lower priority sprites will not be displayed, so red bgcolor!
            bgcolor=QColor(Qt::red);
        }
        if (size16x16){
            pattern = pattern & 0xFC;
            drawMode2CharAt(pattern,   screenx  , screeny  , entry, 0, bgcolor.rgb());
            drawMode2CharAt(pattern+1, screenx+0, screeny+1, entry, 8, bgcolor.rgb());
            drawMode2CharAt(pattern+2, screenx+1, screeny+0, entry, 0, bgcolor.rgb());
            drawMode2CharAt(pattern+3, screenx+1, screeny+1, entry, 8, bgcolor.rgb());
        } else {
            drawMode2CharAt(pattern,   screenx  , screeny  , entry, 0, bgcolor.rgb());
        }
    }else {
        image.fill(QColor(Qt::darkCyan));
    }


}

void VramSpriteView::setPixel2x2(int x, int y, QRgb c)
{
    image.setPixel(2 * x + 0, 2 * y + 0, c);
    image.setPixel(2 * x + 1, 2 * y + 0, c);
    image.setPixel(2 * x + 0, 2 * y + 1, c);
    image.setPixel(2 * x + 1, 2 * y + 1, c);
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

QString VramSpriteView::patterninfo(int character,int spritenr)
{
    QString info;

    //mode 1 then give
    if (spritemode==1 && spritenr>=0){
        info.append(QString("Color data: %1\n").arg(vramBase[attributeTableAddress+spritenr*4+3]));
    };

    //Create header
    info.append("Pattern Data");
    if (size16x16){
        info.append("         ");
    };
    if (spritemode==2){
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
                            hexValue(vramBase[colorTableAddress + spritenr*16 + charrow], 2)
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
                            hexValue(vramBase[colorTableAddress + spritenr*16 + charrow], 2)
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

void VramSpriteView::setCharToDisplay(int character)
{
    if (charToDisplay != character){
        charToDisplay = character;
        refresh();
    };
}

void VramSpriteView::refresh()
{
    //reget pointers in case during boot these pointers weren't correctly set due to openMSx not having send over vram size...
    setVramSource(VDPDataStore::instance().getVramPointer());
    setPaletteSource(VDPDataStore::instance().getPalettePointer());
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
    size16x16 = value;
    refresh();
}

void VramSpriteView::drawCharAt(int character, int x, int y,QRgb fg, QRgb bg)
{
//    printf("drawCharAt vramBase %p patternTableAddress %05x \n",vramBase,patternTableAddress);
    for (int charrow=0 ; charrow<8 ; charrow++){
        unsigned char patternbyte = vramBase[patternTableAddress+8*character+charrow];
        for (int charcol=0 ; charcol<8 ; charcol++){
            unsigned char mask=1<<(7-charcol);
            setPixel2x2(x*8+charcol, y*8+charrow, (patternbyte&mask ? fg : bg ) );
        }
    }
}

void VramSpriteView::drawMode2CharAt(int character, int x, int y, int entry, int rowoffset, QRgb bg)
{
    for (int charrow=0 ; charrow<8 ; charrow++){
        unsigned char patternbyte = vramBase[patternTableAddress+8*character+charrow];
        unsigned char colorbyte = vramBase[colorTableAddress+16*entry+charrow+rowoffset];
        QRgb fg=msxpallet[colorbyte&15]; //drop EC,CC and IC bits
        for (int charcol=0 ; charcol<8 ; charcol++){
            unsigned char mask=1<<(7-charcol);
            setPixel2x2(x*8+charcol, y*8+charrow, (patternbyte&mask ? fg : bg ) );
        }
    }
}
