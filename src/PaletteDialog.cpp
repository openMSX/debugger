#include <QSignalMapper>
#include <QPushButton>
#include <QGridLayout>
#include <QPainter>
#include "PaletteDialog.h"
#include "Convert.h"
#include <algorithm>


PalettePatch::PalettePatch(QWidget* parent, int palNr)
    : QPushButton(parent), msxPalNr(palNr)
{
}

//void PalettePatch::setColor(QRgb c)
//{
//    myColor = c;
//    update();
//}

void PalettePatch::updatePaletteChanged(const uint8_t* pal)
{
    int r = (pal[2 * msxPalNr + 0] & 0xf0) >> 4;
    int b = (pal[2 * msxPalNr + 0] & 0x0f);
    int g = (pal[2 * msxPalNr + 1] & 0x0f);
    auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
    myColor = qRgb(scale(r), scale(g), scale(b));
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    update();
    //printf("PalettePatch::updatePaletteChanged %i\n", msxPalNr);
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
	painter.setPen(isSelected ? Qt::white : QColor(myColor));
	painter.setBrush(QBrush(myColor));
	painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}



PaletteDialog::PaletteDialog(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::PaletteDialog>())
{
    ui->setupUi(this);
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->plainTextEdit->setFont(fixedFont);

    setWindowTitle("Palette editor");
    std::fill(std::begin(myPal), std::end(myPal), 0);
    std::fill(std::begin(myOriginalPal), std::end(myOriginalPal), 0);
    // add colors to colorframe

    signalMapper = new QSignalMapper(this);
    auto* gridLayout = new QGridLayout;
    gridLayout->setSpacing(0);
    for (int i = 0; i < 16; ++i) {
        auto* button = new PalettePatch(nullptr, i);
        connect(button, SIGNAL(clicked(bool)), signalMapper, SLOT(map()));
        signalMapper->setMapping(button, i);
        gridLayout->addWidget(button, i / 8, i % 8);
        connect(this, SIGNAL(paletteChanged(uint8_t*)),
                button, SLOT(updatePaletteChanged(uint8_t*)));
        connect(signalMapper, SIGNAL(mapped(int)),
                button, SLOT(setHighlightTest(int)));
    }

    connect(signalMapper, SIGNAL(mapped(int)),
            this, SLOT(colorSelected(int)));

    ui->colorsframe->setLayout(gridLayout);
    connect(this, SIGNAL(paletteChanged(uint8_t*)),
            this, SLOT(updateText()));
}

void PaletteDialog::updateText()
{
    ui->plainTextEdit->clear();
    for (int i = 0; i < 4; ++i) {
        QString txt(" db ");
        for (int j = 0; j < 4; ++j) {
            txt.append(QString("%1,%2 ")
                .arg(hexValue(myPal[2 * (j + 4 * i) + 0], 2))
                .arg(hexValue(myPal[2 * (j + 4 * i) + 1], 2)));
            if (j < 3) txt.append(',');
        }
        ui->plainTextEdit->appendPlainText(txt);
    }
}

void PaletteDialog::restoreOpeningsPalette()
{
    memcpy(myPal, myOriginalPal, sizeof(myPal));
    emit paletteChanged(myPal); // Resets the PalettePatches
    colorSelected(currentColor); // Resets the sliders
    if (autoSync) {
        syncToSource();
    }
}

void PaletteDialog::colorSelected(int colorNumber)
{
    currentColor = colorNumber;
    int r = (myPal[2 * currentColor + 0] & 0xf0) >> 4;
    int b = (myPal[2 * currentColor + 0] & 0x0f);
    int g = (myPal[2 * currentColor + 1] & 0x0f);
    ui->horizontalSlider_R->setValue(r);
    ui->horizontalSlider_G->setValue(g);
    ui->horizontalSlider_B->setValue(b);
    ui->label_colornr->setText(QString("Color %1").arg(colorNumber));
}

void PaletteDialog::setPalette(uint8_t* pal)
{
    sourcePal = pal;
    memcpy(myOriginalPal, pal, sizeof(myOriginalPal));
    memcpy(myPal, pal, sizeof(myPal));
    emit paletteChanged(myPal);
}

uint8_t* PaletteDialog::getPalette()
{
    return myPal;
}

//void PaletteDialog::decodepalette()
//{
//    for (int i = 0; i < 16; ++i) {
//        int r = (myPal[2 * i + 0] & 0xf0) >> 4;
//        int b = (myPal[2 * i + 0] & 0x0f);
//        int g = (myPal[2 * i + 1] & 0x0f);
//
//        auto scale = [](int x) { return (x >> 1) | (x << 2) | (x << 5); };
//        msxPalette[i] = qRgb(scale(r), scale(g), scale(b));
//
//        QGridLayout* l = dynamic_cast<QGridLayout*>(ui->colorsframe->layout());
//        dynamic_cast<QPushButton*>(l->itemAtPosition(i % 8, i / 8)->widget())->setText(QString("%1(%2%3%4)").arg(i).arg(r).arg(g).arg(b));
//    }
//}

void PaletteDialog::combineRGB()
{
    int r = ui->horizontalSlider_R->value();
    int g = ui->horizontalSlider_G->value();
    int b = ui->horizontalSlider_B->value();
    myPal[2 * currentColor + 0] = 16 * r + b;
    myPal[2 * currentColor + 1] = g;
    emit paletteChanged(myPal);
    if (autoSync) {
        syncToSource();
    }
}

void PaletteDialog::syncToSource()
{
    memcpy(sourcePal, myPal, sizeof(myPal));
    emit paletteSynced();
}

void PaletteDialog::setAutoSync(bool value)
{
    if (autoSync == value) return;
    autoSync = value;
    ui->cb_autosync->setChecked(value);
}

void PaletteDialog::on_horizontalSlider_R_valueChanged(int value)
{
    ui->label_R->setText(QString("R=%1").arg(value));
    combineRGB();
}

void PaletteDialog::on_horizontalSlider_G_valueChanged(int value)
{
    ui->label_G->setText(QString("G=%1").arg(value));
    combineRGB();
}

void PaletteDialog::on_horizontalSlider_B_valueChanged(int value)
{
    ui->label_B->setText(QString("B=%1").arg(value));
    combineRGB();
}

void PaletteDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    if (button== ui->buttonBox->button(QDialogButtonBox::Apply) ||
        button== ui->buttonBox->button(QDialogButtonBox::Ok)) {
        syncToSource();
    } else if (button== ui->buttonBox->button(QDialogButtonBox::Reset) ||
               button== ui->buttonBox->button(QDialogButtonBox::Cancel)) {
        restoreOpeningsPalette();
    }
}

void PaletteDialog::on_cb_autosync_stateChanged(int arg1)
{
    autoSync = arg1 != Qt::Unchecked;
    if (autoSync) {
        syncToSource();
    }
}
