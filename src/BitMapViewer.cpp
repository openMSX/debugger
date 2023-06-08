#include "BitMapViewer.h"
#include "PaletteDialog.h"
#include "VramBitMappedView.h"
#include "VDPDataStore.h"
#include "Convert.h"
#include <QMessageBox>


static uint8_t currentPalette[32] = { 0 };

BitMapViewer::BitMapViewer(QWidget* parent)
	: QDialog(parent)
	, pageSize(0) // avoid UMR
	, screenMod(0)
{
	setupUi(this);
    //no connect slot byname anymore
    connect(screenMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitMapViewer::on_screenMode_currentIndexChanged);
    connect(currentPage, qOverload<int>(&QComboBox:: currentIndexChanged), this, &BitMapViewer::on_currentPage_currentIndexChanged);
    connect(linesVisible, qOverload<int>(&QComboBox:: currentIndexChanged), this, &BitMapViewer::on_linesVisible_currentIndexChanged);
    connect(bgColor,  qOverload<int>(&QSpinBox::valueChanged), this, &BitMapViewer::on_bgColor_valueChanged);

    connect(useVDPRegisters, &QCheckBox::stateChanged,this, &BitMapViewer::on_useVDPRegisters_stateChanged);

    connect(saveImageButton, &QPushButton::clicked, this, &BitMapViewer::on_saveImageButton_clicked);
    connect(editPaletteButton, &QPushButton::clicked, this, &BitMapViewer::on_editPaletteButton_clicked);
    connect(useVDPPalette, &QCheckBox::stateChanged, this, &BitMapViewer::on_useVDPPalette_stateChanged);
    connect(zoomLevel, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &BitMapViewer::on_zoomLevel_valueChanged);

	// hand code entering the actual display widget in the scrollarea With
	// the designer-qt4 there is an extra scrollAreaWidget between the
	// imageWidget and the QScrollArea so the scrollbars are not correctly
	// handled when the image is resized. (since the intermediate widget
	// stays the same size). I did not try to have this intermediate widget
	// resize and all, since it was superflous anyway.
	imageWidget = new VramBitMappedView();
	QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(imageWidget->sizePolicy().hasHeightForWidth());
	imageWidget->setSizePolicy(sizePolicy1);
	imageWidget->setMinimumSize(QSize(256, 212));

	scrollArea->setWidget(imageWidget);

	useVDP = useVDPRegisters->isChecked();

	const unsigned char* vram    = VDPDataStore::instance().getVramPointer();
	imageWidget->setVramSource(vram);
	imageWidget->setVramAddress(0);
	// Palette data not received from VDPDataStore yet causing black image, so
	// we start by using fixed palette until VDPDataStoreDataRefreshed kicks in.
	imageWidget->setPaletteSource(VDPDataStore::instance().getDefaultPalettePointer());
	
	// now hook up some signals and slots
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

void BitMapViewer::updateDisplayAsFrame()
{
	screenMod = screenMode->itemText(screenMode->currentIndex()).toInt();
	pageSize = screenMod >= 7 ? 65536 : 32768;
}

void BitMapViewer::decodeVDPregs()
{
	const unsigned char* regs = VDPDataStore::instance().getRegsPointer();

	// Get the number of lines
	int v1 = (regs[9] & 128) ? 212 : 192;
	printf("\nlines acording to the bits %i,: %i\n", (regs[9] & 128), v1);
	linesLabel->setText(QString("%1").arg(v1, 0, 10));
	if (useVDP) linesVisible->setCurrentIndex((regs[9] & 128) ? 1 : 0);

	// Get the border color
	int v2 = regs[7] & 15;
	printf("\nborder acording to the regs %i,: %i\n", regs[7], v2);
	if (regs[8] & 32) v2 = 0;
	printf("\ncolor 0 is pallet regs %i,: %i\n", (regs[8] & 32), v2);
	borderLabel->setText(QString("%1").arg(v2, 0, 10));
	if (useVDP) bgColor->setValue(v2);

	// Get current screenmode
	static const int bits_modetxt[128] = {
		  1,   3,   0, 255,  2, 255, 255, 255,
		  4, 255,  80, 255,  5, 255, 255, 255,
		  6, 255, 255, 255,  7, 255, 255, 255,
		255, 255, 255, 255,  8, 255, 255, 255,

		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255, 12, 255, 255, 255,

		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,

		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255,255, 255, 255, 255,
		255, 255, 255, 255, 11, 255, 255, 255,
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

	if (useVDP) {
		screenMode->setCurrentIndex(bits_mode[v3]);
		updateDisplayAsFrame();
	}
	if (useVDPPalette) {
		imageWidget->setPaletteSource(VDPDataStore::instance().getPalettePointer());
	}

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
	if (useVDP) currentPage->setCurrentIndex(p);
}

void BitMapViewer::refresh()
{
	// All of the code is in the VDPDataStore;
	VDPDataStore::instance().refresh();
}

void BitMapViewer::on_screenMode_currentIndexChanged(int /*index*/)
{
	updateDisplayAsFrame();

	// change the number of visible pages when this changes
	imageWidget->setVramAddress(0);

	// make sure that we are decoding new mode from page 0, imagine viewing
	// page 3 of screen5 and then switching to screen 8 without changing the
	// starting vram address....
	imageWidget->setScreenMode(screenMod);
	setPages();
}

void BitMapViewer::setPages()
{
	if (pageSize == 0) return;
	static const QStringList pageNames = {"0", "1", "2", "3", "E0", "E1"};
	int oldIndex = currentPage->currentIndex();

	currentPage->clear();
	int pageCount = VDPDataStore::instance().getVRAMSize() / pageSize;
	int pages = std::min(pageCount, pageNames.count());

	for (int p = 0; p < pages; ++p) {
		currentPage->insertItem(p, pageNames[p]);
	}

	if (!useVDP && oldIndex < currentPage->count()) {
		currentPage->setCurrentIndex(oldIndex);
	}
}

void BitMapViewer::on_currentPage_currentIndexChanged(int index)
{
	// if this is the consequence of a .clear() in the
	// on_screenMode_currentIndexChanged we do nothing!
	if (index == -1) return;

	int vramAddress = pageSize * index;
	imageWidget->setVramAddress(vramAddress);
	addressLabel->setText(hexValue(vramAddress, 5));
}

void BitMapViewer::on_linesVisible_currentIndexChanged(int index)
{
	static const int m[3] = { 192, 212, 256 };
	int lines = m[index];
	imageWidget->setLines(lines);
	linesLabel->setText(QString("%1").arg(m[index]));
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
	currentPage->setEnabled(!state);
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
	auto* p = new PaletteDialog();
	p->setPalette(currentPalette);
	p->setAutoSync(true);
	connect(p, &PaletteDialog::paletteSynced, imageWidget, &VramBitMappedView::refresh);
	p->show();
}

void BitMapViewer::on_useVDPPalette_stateChanged(int state)
{
	const uint8_t* palette = VDPDataStore::instance().getPalettePointer();
	if (state) {
		imageWidget->setPaletteSource(palette);
	} else {
		// Copy palette from VDP to allow changes.
		if (palette) memcpy(currentPalette, palette, 32);
		imageWidget->setPaletteSource(currentPalette);
	}
	imageWidget->refresh();
	editPaletteButton->setEnabled(!state);
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
