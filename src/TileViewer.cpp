#include "TileViewer.h"
#include "ui_TileViewer.h"
#include "VramTiledView.h"
#include "VDPDataStore.h"
#include "PaletteDialog.h"
#include "Convert.h"
#include <QMessageBox>


// static to feed to PaletteDialog and be used when VDP colors aren't selected
uint8_t TileViewer::defaultPalette[32] = {
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


TileViewer::TileViewer(QWidget* parent)
    : QDialog(parent), image4label(32, 32, QImage::Format_RGB32)
{
    setupUi(this);
    // No QMetaObject::connectSlotsByName available now since we dropped the slots: in the header;
    connect(cb_tilemapsource, qOverload<int>(&QComboBox::currentIndexChanged), this, &TileViewer::on_cb_tilemapsource_currentIndexChanged);
    connect(cb_screen, qOverload<int>(&QComboBox::currentIndexChanged) ,this ,&TileViewer::on_cb_screen_currentIndexChanged);
    connect(le_nametable, &QLineEdit::textChanged, this, &TileViewer::on_le_nametable_textChanged);
    connect(le_colortable, &QLineEdit::textChanged, this, &TileViewer::on_le_colortable_textChanged);
    connect(le_patterntable, &QLineEdit::textChanged, this, &TileViewer::on_le_patterntable_textChanged);

    connect(cb_color0, &QCheckBox::stateChanged, this, &TileViewer::on_cb_color0_stateChanged);
    connect(useVDPRegisters, &QCheckBox::stateChanged, this, &TileViewer::on_useVDPRegisters_stateChanged);

    connect(editPaletteButton, &QPushButton::clicked, this, &TileViewer::on_editPaletteButton_clicked);
    connect(useVDPPalette, &QCheckBox::stateChanged, this, &TileViewer::on_useVDPPalette_stateChanged);
    connect(zoomLevel, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TileViewer::on_zoomLevel_valueChanged);

    connect(cb_drawgrid, &QCheckBox::stateChanged, this, &TileViewer::on_cb_drawgrid_stateChanged);
    connect(cb_highlight, &QCheckBox::stateChanged, this, &TileViewer::on_cb_highlight_stateChanged);
    connect(sp_highlight, qOverload<int>(&QSpinBox::valueChanged), this, &TileViewer::on_sp_highlight_valueChanged);
    connect(sp_bordercolor, qOverload<int>(&QSpinBox::valueChanged), this, &TileViewer::on_sp_bordercolor_valueChanged);

    connect(cb_blinkcolors, &QCheckBox::stateChanged, this, &TileViewer::on_cb_blinkcolors_stateChanged);
    connect(cb_screenrows, qOverload<int>(&QComboBox::currentIndexChanged), this, &TileViewer::on_cb_screenrows_currentIndexChanged);



    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    plainTextEdit->setFont(fixedFont);
    label_charpat->setFont(fixedFont);
    label_charadr->setFont(fixedFont);

    // Now hook up some signals and slots.
    // This way we have created the VDPDataStore::instance before our imagewidget.
    // This allows the VDPDatastore to start asking for data as quickly as possible.
    connect(refreshButton, &QPushButton::clicked,
            &VDPDataStore::instance(), &VDPDataStore::refresh);
    connect(&VDPDataStore::instance(), &VDPDataStore::dataRefreshed,
            this, &TileViewer::VDPDataStoreDataRefreshed);

    imageWidget = new VramTiledView();
    QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(imageWidget->sizePolicy().hasHeightForWidth());
    imageWidget->setSizePolicy(sizePolicy1);
    imageWidget->setMinimumSize(QSize(256, 212));
    connect(&VDPDataStore::instance(), &VDPDataStore::dataRefreshed,
            imageWidget, &VramTiledView::refresh);

    scrollArea->setWidget(imageWidget);

	const auto* vram    = VDPDataStore::instance().getVramPointer();
	const auto* palette = VDPDataStore::instance().getPalettePointer();
    //imageWidget->setNameTableAddress(0);
    //imageWidget->setPatternTableAddress(0);
    //imageWidget->setColorTableAddress(0);
	imageWidget->setPaletteSource(palette);
    imageWidget->setVramSource(vram);
    imageWidget->setUseBlink(cb_blinkcolors->isChecked());
    imageWidget->setDrawGrid(cb_drawgrid->isChecked());

    connect(imageWidget, &VramTiledView::highlightCount,
            this, &TileViewer::highlightInfo);
    connect(imageWidget, &VramTiledView::imageHovered,
            this, &TileViewer::imageMouseOver);
    connect(imageWidget, &VramTiledView::imageClicked,
            this, &TileViewer::displayCharInfo);

	// and now go fetch the initial data
	VDPDataStore::instance().refresh();
}

void TileViewer::VDPDataStoreDataRefreshed()
{
    if (useVDPPalette->isChecked()) {
        imageWidget->setPaletteSource(VDPDataStore::instance().getPalettePointer());
    }
    decodeVDPregs();
}

void TileViewer::highlightInfo(uint8_t character, int count)
{
    label_highligtinfo->setText(
        (count == 0) ? QString("%1 (not in nametabel)").arg(hexValue(character, 2))
                     : QString("%1 (%2 in nametabel)").arg(hexValue(character, 2)).arg(count));
}

void TileViewer::update_label_characterimage()
{
    imageWidget->drawCharAtImage(mouseOverChar, mouseOverX, mouseOverY, image4label);
    label_characterimage->setPixmap(QPixmap::fromImage(image4label));
}

void TileViewer::imageMouseOver(int screenX, int screenY, int character)
{
    mouseOverX = screenX;
    mouseOverY = screenY;
    mouseOverChar = character;
    update_label_characterimage();

    label_mouseover->setText(QString("col %1  row %2").arg(screenX).arg(screenY));
    int screenMode = imageWidget->getScreenMode();

    // Screen 2 and 4 have 3 pattern/colortables so add 256/512 to get to 2nd and 3th bank depending on y
    //printf("screenmode %i screen y %i character %i \n", screenMode, screenY, character);
    label_charpat->setText(
        (screenMode == 2 || screenMode == 4)
        ? QString("%1/%2 (%3/%4)").arg(hexValue(character & 255, 2)).arg(character >> 8).arg(character & 255).arg(character >> 8)
        : QString("%1 (%2)").arg(hexValue(character, 2)).arg(character));
    imageWidget->drawCharAtImage(character, screenX, screenY, image4label);
    label_characterimage->setPixmap(QPixmap::fromImage(image4label));

    auto nametable = imageWidget->getNameTableAddress();
    auto patterntable = imageWidget->getPatternTableAddress();
    auto colortable = imageWidget->getColorTableAddress();

    QString pgt = hexValue(patterntable + 8 * character, 4);
    QString ct("-");
    //printf("Tileviewer screenmode %i", screenMode);
    switch (screenMode) {
    case 0:
    case 3:
        //no colortable used in these modes
        break;
    case 1:
        ct = hexValue(colortable + (character >> 3), 4);
        break;
    case 2:
    case 4:
        //regular 8 bytes per char colortable, show adr of row 0
        ct = hexValue(colortable + 8 * character, 4);
        break;
    case 80:
        //show the coloraddres for the blink info
        ct = hexValue(colortable + (screenX >> 3) + 10 * screenY, 4);
        break;
    }
    QString nt("");
    if (cb_tilemapsource->currentIndex() > 0) {
        int scrWidth = screenMode == 80 ? 80
                     : screenMode ==  0 ? 40
                                        : 32;
        nt = QString("\nNT: %1 (base+%2) )")
                .arg(hexValue(nametable + scrWidth * screenY + screenX, 4))
                .arg(hexValue(scrWidth * screenY + screenX, 3));
    }
    label_charadr->setText(QString("PGT: %1, CT: %2%3").arg(pgt).arg(ct).arg(nt));
}

void TileViewer::decodeVDPregs()
{
	const uint8_t* regs = VDPDataStore::instance().getRegsPointer();

//	// Get current screenmode
//	static const int bits_modetxt[32] = {
//		  1,   3,   0, 255,  2, 255, 255, 255,
//		  4, 255,  80, 255,  5, 255, 255, 255,
//		  6, 255, 255, 255,  7, 255, 255, 255,
//		255, 255, 255, 255,  8, 255, 255, 255,
//	};

	//int v = ((regs[0] & 0x0E) << 1) | ((regs[1] & 0x10) >> 4) | ((regs[1] & 0x08) >> 2);
	int v = ((regs[0] & 0x0E) << 1) | ((regs[1] & 0x18) >> 3);

    label_M1->setEnabled(v & 0x02);
    label_M2->setEnabled(v & 0x01);
    label_M3->setEnabled(v & 0x04);
    label_M4->setEnabled(v & 0x08);
    label_M5->setEnabled(v & 0x10);

    QString modeText;
    int cbIndex = 6; // "not tile based"
    int reg2mask = 255; // mask for nametable (aka pattern layout table)
    int reg3mask = 255; // mask for colortable
    int reg4mask = 255; // mask for patterntable
    bool usesBlink = false;
    switch (v) {
        case  0: modeText = "Graphic1";   cbIndex = 2; break;
        case  1: modeText = "MultiColor"; cbIndex = 4; break;
        case  2: modeText = "Text1";      cbIndex = 0; reg3mask = 0x80; break;
        case  4: modeText = "Graphic2";   cbIndex = 3; reg3mask = 0x80; reg4mask = 0x3c; break;
        case  8: modeText = "Graphic3";   cbIndex = 5; reg3mask = 0x80; reg4mask = 0x3c; break;
        case 10: modeText = "Text2";      cbIndex = 1; reg3mask = 0xf8; reg2mask = 0xfc; usesBlink = true; break;
        case 12: modeText = "Graphic4"; break;
        case 16: modeText = "Graphic5"; break;
        case 20: modeText = "Graphic6"; break;
        case 28: modeText = "Graphic7"; break;
        default: modeText = "unknown";  break;
    }
    label_screenmode->setText(modeText);

    if (useVDPRegisters->isChecked()) {
        cb_screen->setCurrentIndex(cbIndex);
        int nameTable = (regs[2] & reg2mask) << 10;
        int patternTable = (regs[4] & reg4mask) << 11;
        int colorTable = ((regs[3] & reg3mask) << 6) | (regs[10] << 14);
        le_nametable->setText(hexValue(nameTable, 4));
        le_patterntable->setText(hexValue(patternTable, 4));
        le_colortable->setText(hexValue(colorTable, 4));
        cb_blinkcolors->setChecked(usesBlink ? regs[13] & 0xf0 : false);
        cb_screenrows->setCurrentIndex((regs[9] & 128) ? 1 : 0 );
        sp_bordercolor->setValue(regs[7] & 15);
        cb_color0->setChecked(regs[8] & 32);
    }
}

void TileViewer::refresh()
{
	// All of the code is in the VDPDataStore;
    VDPDataStore::instance().refresh();
}

void TileViewer::displayCharInfo(int screenX, int screenY, int character, const QString& textInfo)
{
    label_charpat->setText(QString::number(character));
    plainTextEdit->setPlainText(QString("info for character %1 (%2)\n%3")
                                .arg(character)
                                .arg(hexValue(character, 2))
                                .arg(textInfo));
    imageWidget->drawCharAtImage(character, screenX, screenY, image4label);
    label_clickedcharimage->setPixmap(QPixmap::fromImage(image4label));
}

void TileViewer::on_cb_tilemapsource_currentIndexChanged(int index)
{
    imageWidget->setTabletoShow(index);
}

void TileViewer::on_cb_screen_currentIndexChanged(int index)
{
	int i = [&] {
		switch (index) {
            case 0:  return   0;
            case 1:  return  80;
            case 2:  return   1;
            case 3:  return   2;
            case 4:  return   3;
            case 5:  return   4;
            case 6:  return 255;
            default: return   0;
        }
	}();
	imageWidget->setScreenMode(i);
	// blink checkbox depends on screenMode and manuel usage
	cb_blinkcolors->setEnabled(i == 80 && !useVDPRegisters->isChecked());
}

void TileViewer::on_le_nametable_textChanged(const QString& text)
{
    auto font = le_nametable->font();
    if (auto i = stringToValue<int>(text)) {
        imageWidget->setNameTableAddress(*i);
        font.setItalic(false);
    } else {
        font.setItalic(true);
    }
    le_nametable->setFont(font);
}

void TileViewer::on_le_colortable_textChanged(const QString& text)
{
    auto font = le_colortable->font();
    if (auto i = stringToValue<int>(text)) {
        imageWidget->setColorTableAddress(*i);
        font.setItalic(false);
    } else {
        font.setItalic(true);
    }
    le_colortable->setFont(font);
}

void TileViewer::on_le_patterntable_textChanged(const QString& text)
{
    auto font = le_patterntable->font();
    if (auto i = stringToValue<int>(text)) {
        imageWidget->setPatternTableAddress(*i);
        font.setItalic(false);
    } else {
        font.setItalic(true);
    }
    le_patterntable->setFont(font);
}

void TileViewer::on_cb_color0_stateChanged(int state)
{
	imageWidget->setTpBit(state != Qt::Unchecked);
}

void TileViewer::on_useVDPRegisters_stateChanged(int state)
{
    bool useManual = state == Qt::Unchecked;
    const uint8_t* regs = VDPDataStore::instance().getRegsPointer();
    decodeVDPregs();
    cb_screen->setEnabled(useManual);
    le_colortable->setEnabled(useManual);
    le_nametable->setEnabled(useManual);
    le_patterntable->setEnabled(useManual);
    cb_screenrows->setEnabled(useManual);
    cb_screenrows->setCurrentIndex(regs[9] & 128 ? 1 : 0);
    sp_bordercolor->setEnabled(useManual);
    int v = ((regs[0] & 0x0E) << 1) | ((regs[1] & 0x18) >> 3);
    cb_blinkcolors->setEnabled(useManual && v == 10);  // screenMode == 80
    cb_color0->setEnabled(useManual);
}

void TileViewer::on_editPaletteButton_clicked(bool /*checked*/)
{
    auto* p = new PaletteDialog();
    p->setPalette(defaultPalette);
    p->setAutoSync(true);
    connect(p, &PaletteDialog::paletteSynced, imageWidget, &VramTiledView::refresh);
    connect(p, &PaletteDialog::paletteSynced, this, &TileViewer::update_label_characterimage);
    p->show();
    //useVDPPalette->setChecked(false);
    //QMessageBox::information(
    //    this,
    //    "Not yet implemented",
    //    "Sorry, the palette editor is not yet implemented, "
    //    "only disabling 'Use VDP palette registers' for now");
}

void TileViewer::on_useVDPPalette_stateChanged(int state)
{
    const uint8_t* palette = VDPDataStore::instance().getPalettePointer();
    if (state) {
        imageWidget->setPaletteSource(palette);
    } else {
        if (palette) memcpy(defaultPalette, palette, 32);
        imageWidget->setPaletteSource(defaultPalette);
    }
    imageWidget->refresh();
    editPaletteButton->setEnabled(!state);
}

void TileViewer::on_zoomLevel_valueChanged(double d)
{
    imageWidget->setZoom(float(d));
}

void TileViewer::on_cb_drawgrid_stateChanged(int state)
{
    imageWidget->setDrawGrid(state == Qt::Checked);
}

void TileViewer::on_cb_highlight_stateChanged(int state)
{
    imageWidget->setHighlightChar(state == Qt::Unchecked ? -1 : sp_highlight->value());
}

void TileViewer::on_sp_highlight_valueChanged(int i)
{
    if (cb_highlight->isChecked()) {
        imageWidget->setHighlightChar(i);
    }
}

void TileViewer::on_sp_bordercolor_valueChanged(int i)
{
    imageWidget->setBorderColor(i);
}

void TileViewer::on_cb_blinkcolors_stateChanged(int arg1)
{
    imageWidget->setUseBlink(arg1 == Qt::Checked);
}

void TileViewer::on_cb_screenrows_currentIndexChanged(int index)
{
    auto rows = cb_screenrows->isEnabled() ? (index == 0 ? 24.0f :
                                              index == 1 ? 26.5f
                                                         : 32.0f)
                                           : 0.0f;
    imageWidget->setForcedScreenRows(rows);
}
