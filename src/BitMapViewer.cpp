#include "BitMapViewer.h"
#include "VramBitMappedView.h"
#include <QMessageBox>
#include "VDPDataStore.h"


BitMapViewer::BitMapViewer( QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	// hand code entering the actual display widget in the scrollarea
	// With the designer-qt4 there is an extra scrollAreaWidget between 
	// the imageWidget and the QScrollArea so the scrollbars are not 
	// correctly handled when the image is resized. (since the intermediate widget stays the same size)
	// I did not try to have this intermediate widget resize and all, since it was superflous anyway.
	imageWidget = new VramBitMappedView();
	QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(imageWidget->sizePolicy().hasHeightForWidth());
	imageWidget->setSizePolicy(sizePolicy1);
	imageWidget->setMinimumSize(QSize(256, 212));

	scrollArea->setWidget(imageWidget);

	lines=212;
	vramSize=lines*256;
	useVDP=true;
	useVDPcolors=true;
	vram = VDPDataStore::instance().getVramPointer();
	palette = VDPDataStore::instance().getPalettePointer();
	regs =  VDPDataStore::instance().getRegsPointer();
	vramAddress=0;
	imageWidget->setVramSource(vram);
	imageWidget->setVramAddress(0);
	imageWidget->setPaletteSource(palette);

	//now hook up some signals and slots
	connect( &VDPDataStore::instance(), SIGNAL( dataRefreshed() ), this, SLOT( VDPDataStoreDataRefreshed() ) );
	connect( &VDPDataStore::instance(), SIGNAL( dataRefreshed() ), imageWidget, SLOT( refresh() ) );
	connect( refreshButton,  SIGNAL( clicked (bool) ), &VDPDataStore::instance(), SLOT( refresh() ) );

	connect( imageWidget,  SIGNAL( imagePosition (int,int,int,unsigned int,int) ), this, SLOT( imagePositionUpdate(int,int,int,unsigned int,int) ) );
	connect( imageWidget,  SIGNAL( imageClicked (int,int,int,unsigned int,int) ), this, SLOT( imagePositionUpdate(int,int,int,unsigned int,int) ) );

	// and now go fetch the initial data
	VDPDataStore::instance().refresh();
}

BitMapViewer::~BitMapViewer()
{
}

void BitMapViewer::setDefaultPalette()
{
	int rgb[][3]={
		{0,0,0},
		{0,0,0},
		{1,6,1},
		{3,7,3},
		{1,1,7},
		{2,3,7},
		{5,1,1},
		{2,6,7},
		{7,1,1},
		{7,3,3},
		{6,6,1},
		{6,6,4},
		{1,4,1},
		{6,2,5},
		{5,5,5},
		{7,7,7}
		};
	for (int i; i<16;i++){
		palette[2*i]= ((rgb[i][0])<<4) + rgb[i][2];
		palette[2*i +1]= rgb[i][1];
	}
}

void BitMapViewer::decodeVDPregs()
{
	int v;
	//Get the number of lines from bit x from reg Y
	v = (regs[9]&128)?212:192;
	printf("\nlines acording to the bits %i,: %i\n",(regs[9]&128),v);
	linesLabel->setText(QString("%1").arg(v,0,10));
	if (useVDP) linesVisible->setCurrentIndex( (regs[9]&128)?1:0 );
	
	// Get the border color from register 7
	v = 15 & regs[7];
	printf("\nborder acording to the regs %i,: %i\n",(regs[7]),v);
	if (regs[8]&32) v=0;
	printf("\ncolor 0 is pallet regs %i,: %i\n",(regs[8]&32),v);
	borderLabel->setText(QString("%1").arg(v,0,10));
	if (useVDP) bgColor->setValue(v);

	//Get current screenmode
	v=((regs[0]&14)<<1) |((regs[1]&24)>>3);
	printf("screenMod according to the bits: %i\n",v);
	int bits_modetxt[]={   1,  3,  0,255,  2,255,255,255,
			       4,255, 80,255,  5,255,255,255,
			       6,255,255,255,  7,255,255,255,
			     255,255,255,255,  8,255,255,255
			};
	modeLabel->setText(QString("%1").arg(bits_modetxt[v],0,10));
	int bits_mode[]={ 0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  1,0,0,0,0,0,0,0,
			  0,0,0,0,3,0,0,0
			};
	if (useVDP) screenMode->setCurrentIndex(bits_mode[v]);

	//get the current visible page
	unsigned p = (regs[2]>>5)&3;
	unsigned q = 1<<15;
	if (bits_modetxt[v]>6)  { p&=1; q<<=1 ;};
	printf("visible page according to the bits: %i\n",p);
	addressLabel->setText(QString("0x%1").arg(p*q,5,16,QChar('0')));
	if (useVDP) showPage->setCurrentIndex(p);

}


void BitMapViewer::refresh()
{
	// All of the code is in the VDPDataStore;
	VDPDataStore::instance().refresh();
}

void BitMapViewer::on_screenMode_currentIndexChanged( const QString & text  )
{
	screenMod=text.toInt();
	printf("\nnew screenMod: %i\n",screenMod);
	//change then number of visibe pages when this changes
	vramAddress=0;
	vramSize=lines*256;
	imageWidget->setVramAddress(vramAddress);
	// make sure that we are decoding new mode from page 0, imagine viewing
	// page 3 of screen5 and then switching to screen 8 without changing the
	// starting vram address....
	imageWidget->setScreenMode(screenMod);
	showPage->clear();
	showPage->insertItem(0, "0");
	showPage->insertItem(1, "1");
	if (screenMod < 7){
		vramSize=lines*128;
		showPage->insertItem(2, "2");
		showPage->insertItem(3, "3");
	}
}

void BitMapViewer::on_showPage_currentIndexChanged( int index )
{
	//if this is the consequence of a a .clear() in the on_screenMode_currentIndexChanged
	// then we do nothing!
	if (index == -1) return;
	showpage=index;

	int m1[] = {0x00000 , 0x08000, 0x10000, 0x18000 };
	printf("\nvoid BitMapViewer::on_showPage_currentIndexChanged( int %i);\n",index);
	if (screenMod > 7 ) index *= 2;
	vramAddress = m1[index];
	printf("vramAddress %i\n",vramAddress);
	imageWidget->setVramAddress(vramAddress);

}

void BitMapViewer::on_linesVisible_currentIndexChanged( int index )
{
	int m[] = {192,212,256};
	lines = m[index];
	vramSize=lines*((screenMod < 7)?128:256);
	imageWidget->setLines(lines);
}

void BitMapViewer::on_bgColor_valueChanged ( int value )
{
	imageWidget->setBorderColor(value);
}

void BitMapViewer::on_useVDPRegisters_stateChanged ( int state )
{
	useVDP = state;

	screenMode->setEnabled(!state);
	linesVisible->setEnabled(!state);
	showPage->setEnabled(!state);
	bgColor->setEnabled(!state);
	decodeVDPregs();
	imageWidget->refresh();
}

void BitMapViewer::on_zoomLevel_valueChanged ( double d )
{
	imageWidget->setZoom(float(d));
}

void BitMapViewer::on_saveImageButton_clicked( bool checked )
{
	QMessageBox::information(this,"Not yet implemented","Sorry, the save image dialog is not yet implemented");
}

void BitMapViewer::on_editPaletteButton_clicked( bool checked )
{
	useVDPPalette->setChecked(false); 
	QMessageBox::information(this,"Not yet implemented","Sorry, the palette editor is not yet implementedi,only disabling 'Use VDP palette registers' for now");
}

void BitMapViewer::on_useVDPPalette_stateChanged ( int state )
{
	useVDPcolors = state;
	if (!state) setDefaultPalette();
	//refresh();
	imageWidget->refresh();
}
/*
void BitMapViewer::on_refreshButton_clicked( bool checked )
{
	refresh();
	decodeVDPregs();
	imageWidget->refresh();
}
*/

void BitMapViewer::VDPDataStoreDataRefreshed()
{
	decodeVDPregs();
}

void BitMapViewer::imagePositionUpdate(int x,int y,int color, unsigned addr, int byteValue)
{
	labelX->setText(QString("%1").arg(x,3,10,QChar('0')));
	labelY->setText(QString("%1").arg(y,3,10,QChar('0')));
	labelColor->setText(QString("%1").arg(color,3,10,QChar('0')));
	labelByte->setText(QString("0x%1").arg(byteValue,2,16,QChar('0')));
	labelVramAddr->setText(QString("0x%1").arg(addr,5,16,QChar('0')));

}
