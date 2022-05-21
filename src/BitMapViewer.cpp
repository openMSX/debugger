#include "BitMapViewer.h"
#include "VramBitMappedView.h"
#include "VDPDataStore.h"
#include "Convert.h"
#include <QMessageBox>

static const uint8_t defaultPalette[32] = {
//        RB  G
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

BitMapViewer::BitMapViewer(QWidget* parent)
	: QDialog(parent)
	, screenMod(0) // avoid UMR
{
	setupUi(this);
    //no connect slot byname anymore
    connect(screenMode, QOverload<int>::of(&QComboBox:: currentIndexChanged), this, &BitMapViewer::on_screenMode_currentIndexChanged);
    connect(showPage, QOverload<int>::of(&QComboBox:: currentIndexChanged), this, &BitMapViewer::on_showPage_currentIndexChanged);
    connect(linesVisible, QOverload<int>::of(&QComboBox:: currentIndexChanged), this, &BitMapViewer::on_linesVisible_currentIndexChanged);
    connect(bgColor,  QOverload<int>::of(&QSpinBox::valueChanged), this, &BitMapViewer::on_bgColor_valueChanged);

    connect(useVDPRegisters, &QCheckBox::stateChanged,this, &BitMapViewer::on_useVDPRegisters_stateChanged);

    connect(saveImageButton, &QPushButton::clicked, this, &BitMapViewer::on_saveImageButton_clicked);
    connect(editPaletteButton, &QPushButton::clicked, this, &BitMapViewer::on_editPaletteButton_clicked);
    connect(useVDPPalette, &QCheckBox::stateChanged, this, &BitMapViewer::on_useVDPPalette_stateChanged);
    connect(zoomLevel, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &BitMapViewer::on_zoomLevel_valueChanged);

	// hand code entering the actual display widget in the scrollarea With
	// the designer-qt4 there is an extra scrollAreaWidget between the
	// imageWidget and the QScrollArea so the scrollbars are not correctly
	// handled when the image is resized. (since the intermediate widget
	// stays the same size). I did not try to have this intermediate widget
	// resize and all, since it was superfluous anyway.
	imageWidget = new VramBitMappedView();
	QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(imageWidget->sizePolicy().hasHeightForWidth());
	imageWidget->setSizePolicy(sizePolicy1);
	imageWidget->setMinimumSize(QSize(256, 212));

	scrollArea->setWidget(imageWidget);

	useVDP = useVDPRegisters->isChecked();

	const uint8_t* vram    = VDPDataStore::instance().getVramPointer();
	const uint8_t* palette = VDPDataStore::instance().getPalettePointer();
	imageWidget->setVramSource(vram);
	imageWidget->setVramAddress(0);
	imageWidget->setPaletteSource(palette);

	//now hook up some signals and slots
	connect(&VDPDataStore::instance(), &VDPDataStore::dataRefreshed,
	        this, &BitMapViewer::VDPDataStoreDataRefreshed);
	connect(&VDPDataStore::instance(), &VDPDataStore::dataRefreshed,
	        imageWidget, &VramBitMappedView::refresh);
	connect(refreshButton, &QPushButton::clicked,
	        &VDPDataStore::instance(), &VDPDataStore::refresh);

	connect(imageWidget, &VramBitMappedView::imageHovered,
	        this, &BitMapViewer::updateImagePosition);
	connect(imageWidget, &VramBitMappedView::imageClicked,
	        this, &BitMapViewer::updateImagePosition);

	// and now go fetch the initial data
	VDPDataStore::instance().refresh();
}

void BitMapViewer::decodeVDPregs()
{
	const uint8_t* regs = VDPDataStore::instance().getRegsPointer();

	// Get the number of lines
	int v1 = (regs[9] & 128) ? 212 : 192;
	printf("\nlines according to the bits %i,: %i\n", (regs[9] & 128), v1);
	linesLabel->setText(QString("%1").arg(v1, 0, 10));
	if (useVDP) linesVisible->setCurrentIndex((regs[9] & 128) ? 1 : 0);

	// Get the border color
	int v2 = regs[7] & 15;
	printf("\nborder according to the regs %i,: %i\n", regs[7], v2);
	if (regs[8] & 32) v2 = 0;
	printf("\ncolor 0 is pallet regs %i,: %i\n", (regs[8] & 32), v2);
	borderLabel->setText(QString("%1").arg(v2, 0, 10));
	if (useVDP) bgColor->setValue(v2);

	// Get current screenmode
	static const int bits_modetxt[128] = {
		  1,   3,   0, 255,   2, 255, 255, 255,
		  4, 255,  80, 255,   5, 255, 255, 255,
		  6, 255, 255, 255,   7, 255, 255, 255,
		255, 255, 255, 255,   8, 255, 255, 255,

		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255,  12, 255, 255, 255,

		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,

		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255,  11, 255, 255, 255,
	};
	static const int bits_mode[128] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 2, 0, 0, 0,
		0, 0, 0, 0, 3, 0, 0, 0,

		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 6, 0, 0, 0,

		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,

		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 5, 0, 0, 0,
	};
	int v3 = ((regs[0] & 0x0E) << 1) | ((regs[1] & 0x18) >> 3) | ((regs[25] & 0x18) << 2);
	printf("screenMod according to the bits: %i\n", v3);
	modeLabel->setText(QString("%1").arg(bits_modetxt[v3], 0, 10));
	if (useVDP) screenMode->setCurrentIndex(bits_mode[v3]);

	// Get the current visible page
	unsigned p = (regs[2] >> 5) & 3;
	unsigned q = 1 << 15;
	if (bits_modetxt[v3] > 6) {
		p &= 1;
		q <<= 1;
	}
	printf("visible page according to the bits: %i\n",p);

	setPages();

	addressLabel->setText(hexValue(p * q, 5));
	if (useVDP) showPage->setCurrentIndex(p);
}


void BitMapViewer::refresh()
{
	// All of the code is in the VDPDataStore;
	VDPDataStore::instance().refresh();
}

void BitMapViewer::on_screenMode_currentIndexChanged(int index)
{
    screenMod = screenMode->itemText(index).toInt();
	printf("\nnew screenMod: %i\n", screenMod);
	//change then number of visibe pages when this changes
	imageWidget->setVramAddress(0);
	// make sure that we are decoding new mode from page 0, imagine viewing
	// page 3 of screen5 and then switching to screen 8 without changing the
	// starting vram address....
	imageWidget->setScreenMode(screenMod);
	setPages();
}

void BitMapViewer::setPages()
{
	showPage->clear();
	showPage->insertItem(0, "0");
	showPage->insertItem(1, "1");
	if (screenMod < 7 && VDPDataStore::instance().getVRAMSize() > 0x10000) {
		showPage->insertItem(2, "2");
		showPage->insertItem(3, "3");
	}
}

void BitMapViewer::on_showPage_currentIndexChanged(int index)
{
	//if this is the consequence of a .clear() in the
	// on_screenMode_currentIndexChanged we do nothing!
	if (index == -1) return;

	static const int m1[4] = { 0x00000, 0x08000, 0x10000, 0x18000 };
	printf("\nvoid BitMapViewer::on_showPage_currentIndexChanged(int %i);\n", index);
	if (screenMod >= 7) index *= 2;
	int vramAddress = m1[index];
	printf("vramAddress %i\n", vramAddress);
	imageWidget->setVramAddress(vramAddress);
}

void BitMapViewer::on_linesVisible_currentIndexChanged(int index)
{
	static const int m[3] = { 192, 212, 256 };
	int lines = m[index];
	imageWidget->setLines(lines);
}

void BitMapViewer::on_bgColor_valueChanged(int value)
{
	imageWidget->setBorderColor(value);
}

void BitMapViewer::on_useVDPRegisters_stateChanged(int state)
{
	useVDP = state;

	screenMode->setEnabled(!state);
	linesVisible->setEnabled(!state);
	showPage->setEnabled(!state);
	bgColor->setEnabled(!state);
	decodeVDPregs();
	imageWidget->refresh();
}

void BitMapViewer::on_zoomLevel_valueChanged(double d)
{
	imageWidget->setZoom(float(d));
}

void BitMapViewer::on_saveImageButton_clicked(bool /*checked*/)
{
	QMessageBox::information(
		this,
		"Not yet implemented",
		"Sorry, the save image dialog is not yet implemented");
}

void BitMapViewer::on_editPaletteButton_clicked(bool /*checked*/)
{
	useVDPPalette->setChecked(false);
	QMessageBox::information(
		this,
		"Not yet implemented",
		"Sorry, the palette editor is not yet implemented, "
		"only disabling 'Use VDP palette registers' for now");
}

void BitMapViewer::on_useVDPPalette_stateChanged(int state)
{
	if (state) {
		const uint8_t* palette = VDPDataStore::instance().getPalettePointer();
		imageWidget->setPaletteSource(palette);
	} else {
		imageWidget->setPaletteSource(defaultPalette);
	}
	imageWidget->refresh();
}
/*
void BitMapViewer::on_refreshButton_clicked(bool checked)
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

void BitMapViewer::updateImagePosition(
	int x, int y, int color, unsigned addr, int byteValue)
{
	labelX->setText(QString("%1").arg(x, 3, 10, QChar('0')));
	labelY->setText(QString("%1").arg(y, 3, 10, QChar('0')));
	labelColor->setText(QString("%1").arg(color, 3, 10, QChar('0')));
	labelByte->setText(hexValue(byteValue, 2));
	labelVramAddr->setText(hexValue(addr, 5));
}
