#include "VramBitMappedView.h"
#include <QPainter>
#include <stdio.h>

/** Clips x to the range [LO,HI].
  * Slightly faster than    std::min(HI, std::max(LO, x))
  * especially when no clipping is required.
  */
template <int LO, int HI>
inline int clip(int x)
{
        return unsigned(x - LO) <= unsigned(HI - LO) ? x : (x < HI ? LO : HI);
}



VramBitMappedView::VramBitMappedView(QWidget *parent) : QWidget(parent),image(512,512,QImage::Format_RGB32)
{
  lines=212;
  screenMode=5;
  borderColor=0;
  pallet =  NULL;
  vramBase = NULL;
  vramAddress = 0;
  for (int i=0 ; i<15 ;i++){
			msxpallet[i] = qRgb(80,80,80);
  }
  setZoom(1.0);

  // mouse update events when mouse is moved over the image, Quibus likes this
  // better then my preferd click-on-the-image
  setMouseTracking(true);
}

void VramBitMappedView::setZoom(float zoom)
{
  if (zoom<1.0) zoom=1.0;
  setFixedSize(int(512*zoom), int(lines*2*zoom));
  zoomFactor=zoom;
  update();
}

void VramBitMappedView::decode()
{
	if (!vramBase) return;

	printf("\n"
	       "screenMode: %i\n"
	       "vram to start decoding: %i\n",
	       screenMode, vramAddress);
	switch (screenMode) {
	case 12:
		decodeSCR12();
		break;
	case 11:
		decodeSCR11();
		break;
	case 10:
		decodeSCR10();
		break;
	case 8:
		decodeSCR8();
		break;
	case 7:
		decodeSCR7();
		break;
	case 6:
		decodeSCR6();
		break;
	case 5:
		decodeSCR5();
		break;
	}
	piximage = piximage.fromImage(image);
	update();
}


void VramBitMappedView::decodePallet()
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

void VramBitMappedView::decodeSCR12()
{
  const unsigned char* val;
  int offset=vramAddress;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
                unsigned p[4];
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[0] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[1] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[2] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[3] = *val ;

                int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
                int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);

                for (unsigned n = 0; n < 4; ++n) {
                        int z = p[n] >> 3;
                        int r = clip<0, 31>(z + j);
                        int g = clip<0, 31>(z + k);
                        int b = clip<0, 31>((5 * z - 2 * j - k) / 4);
			
			r = (r<<3) | (r>>2) ;
			b = (b<<3) | (b>>2) ;
			g = (g<<3) | (g>>2) ;

			QRgb c = qRgb(r,g,b);
			image.setPixel(x  , y++, c );
			image.setPixel(x++, y  , c );
			image.setPixel(x  , y--, c );
			image.setPixel(x++, y  , c );
                }
      }
      y += 2;
    }
}

void VramBitMappedView::decodeSCR11()
{
	//actually screen 11 and 10 are technically the same, only the behavior of BASIC is different
	decodeSCR10();
}
void VramBitMappedView::decodeSCR10()
{
  const unsigned char* val;
  int offset=vramAddress;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
                unsigned p[4];
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[0] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[1] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[2] = *val ;
		val = vramBase + (offset>>1) +
			( ( offset & 1 ) ? 0x10000 : 0 );
		offset++;
                p[3] = *val ;

                int j = (p[2] & 7) + ((p[3] & 3) << 3) - ((p[3] & 4) << 3);
                int k = (p[0] & 7) + ((p[1] & 3) << 3) - ((p[1] & 4) << 3);

                for (unsigned n = 0; n < 4; ++n) {
                        QRgb c;
                        if (p[n] & 0x08) {
                                // YAE
                                c = msxpallet[ p[n]>>4 ];
			} else {
				// YJK
				int z = p[n] >> 3;
				int r = clip<0, 31>(z + j);
				int g = clip<0, 31>(z + k);
				int b = clip<0, 31>((5 * z - 2 * j - k) / 4);
				r = (r<<3) | (r>>2) ;
				b = (b<<3) | (b>>2) ;
				g = (g<<3) | (g>>2) ;

				c = qRgb(r,g,b);
			}
			image.setPixel(x  , y++, c );
			image.setPixel(x++, y  , c );
			image.setPixel(x  , y--, c );
			image.setPixel(x++, y  , c );
                }
      }
      y += 2; 
    }
}

void VramBitMappedView::decodeSCR8()
{
  const unsigned char* val;
  int offset=vramAddress;

  int r,g,b;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
	val = vramBase + (offset>>1) +
		( ( offset & 1 ) ? 0x10000 : 0 );
	b = (*val) & 0x03;
	b = b| (b<<2) | (b<<4) | (b<<6) ;
	r = (*val) & 0x1C;
	r = (r>>2) | r | (r<<3);
	g = (*val) & 0xE0;
	g = g | (g>>3) | (g>>6);

	QRgb c = qRgb(r,g,b);
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	offset++;
      }
      y += 2;
    }
}

void VramBitMappedView::decodeSCR7()
{
  const unsigned char* val = vramBase + vramAddress;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
	int v=15&((*val)>>4) ;
	QRgb c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	v=15&(*val) ;
	c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	val++;
      }
      y += 2;
    }
}

void VramBitMappedView::decodeSCR6()
{
  const unsigned char* val = vramBase + vramAddress;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
	int v=  3&((*val)>>6);
	QRgb c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	v=  3&((*val)>>4);
	c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	v=  3&((*val)>>2);
	c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	v=  3&(*val);
	c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	val++;
      }
      y += 2;
    }
}

void VramBitMappedView::decodeSCR5()
{
  const unsigned char* val = vramBase + vramAddress;

    for (int y=0;y<lines*2;){
      for (int x=0;x<511;){
	int v=15&((*val)>>4) ;
	QRgb c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	v=15&(*val) ;
	c = msxpallet[ v?v:borderColor ];
	image.setPixel(x  , y++, c );
	image.setPixel(x++, y  , c );
	image.setPixel(x  , y--, c );
	image.setPixel(x++, y  , c );
	val++;
      }
      y += 2;
    }
}

void VramBitMappedView::paintEvent( QPaintEvent *event )
{
	QRect srcRect(0,0,511,2*lines);
	QRect dstRect(0,0,511*zoomFactor,2*lines*zoomFactor);
	QPainter qp(this);
	//qp.drawImage(rect(),image,srcRect);
	qp.drawPixmap(dstRect,piximage,srcRect);
	qp.end();
}

void VramBitMappedView::refresh()
{
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::mouseMoveEvent ( QMouseEvent * e )
{
	const unsigned int bytes_a_line[]={0,	//screen 0
					1,	//screen 1
					2,	// 2
					3,	// 3
					4,	
					128,	// 5
					128,
					256,	// 7
					256,
					256,	// 9
					256,
					256,
					256 };
	const unsigned int pixels_per_byte[]={0,1,2,3,4,
					2,	// 5
					4,
					2,	// 7
					1,
					1,	//9
					1,
					1,
					1 };
	/*
	const unsigned int bytes_a_page[]={0,1,2,3,4,
					256*128, // 5
					256*128,
					256*256, // 7
					256*256,
					256*256, // 9
					256*256,
					256*256,
					256*256 };
	*/

	int x = int(e->x()/zoomFactor);
	int y = int(e->y()/zoomFactor) >> 1;
	if (!( (screenMode==6) || (screenMode==7) )){
		x=x>>1;
	};

	unsigned int offset = bytes_a_line[screenMode] * y + \
			x/pixels_per_byte[screenMode];
	unsigned int addr = offset + vramAddress ;
	const unsigned char* val = vramBase +
		((screenMode < 8) ? addr
		                  : (((addr >> 1) | (addr << 16)) & 0x1FFFF));

	int byteval = *val;
	int color;
	switch (screenMode){
		case 5:
		case 7:
			color = 15 & ((x&1)?byteval:(byteval>>4));
			break;;
		case 6:
			color = 3 & (byteval>>(2*(3-(x&3))));
			break;;
		case 8:
		case 10:
		case 11:
		case 12:
			color=byteval;

	}
	emit imagePosition(x,y,color,addr,byteval);
}

void VramBitMappedView::mousePressEvent ( QMouseEvent * e )
{
	// since mouseMove only emits the correct signal we reuse/abuse that method
	mouseMoveEvent(e);

	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setBorderColor(int value)
{
	if (value<0) value=0;
	if (value>15) value=15;
	borderColor = value;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setScreenMode(int mode)
{
	screenMode=mode;
	decode();
	update();
}

void VramBitMappedView::setLines(int nrLines)
{
	//lines = 255 & (nrLines - 1);
	lines = nrLines ;
	decode();
	setFixedSize(int(512*zoomFactor), int(lines*2*zoomFactor));
	update();
	//setZoom(zoomFactor);
}

void VramBitMappedView::setVramAddress(int adr)
{
	vramAddress = adr;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setVramSource(const unsigned char* adr)
{
	vramBase = adr;
	decodePallet();
	decode();
	update();
}

void VramBitMappedView::setPaletteSource(const unsigned char* adr)
{
	pallet = adr;
	decodePallet();
	decode();
	update();
}

