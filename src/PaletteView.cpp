#include "PaletteView.h"
#include "PalettePatch.h"
#include "VDPDataStore.h"
#include "Convert.h"
#include "ScopedAssign.h"
#include "ui_PaletteView.h"
#include <QPushButton>
#include <QJsonObject>

PaletteView::PaletteView(QWidget* parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::PaletteView>())
{
    ui->setupUi(this);
    autoSync = ui->cb_autosync->isChecked();

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->plainTextEdit->setFont(fixedFont);

    setPalette(VDPDataStore::instance().getPalette(paletteVDP));

    signalMapper = new QSignalMapper(this);
    auto* gridLayout = new QGridLayout;
    gridLayout->setSpacing(0);
    for (int i = 0; i < 16; ++i) {
        auto* button = new PalettePatch(nullptr, i);
        button->setMSXPalette(myPal);
        connect(button, &PalettePatch::clicked, signalMapper, qOverload<>(&QSignalMapper::map));
        signalMapper->setMapping(button, i);
        gridLayout->addWidget(button, i / 8, i % 8);
        connect(this, &PaletteView::paletteReplaced, button, &PalettePatch::setMSXPalette);
        connect(this, &PaletteView::paletteChanged, button, &PalettePatch::paletteChanged);
        connect(signalMapper, &QSignalMapper::mappedInt, button, &PalettePatch::setHighlightTest);
    }

    connect(signalMapper, &QSignalMapper::mappedInt, this, &PaletteView::colorSelected);

    ui->colorsframe->setLayout(gridLayout);
    connect(this, &PaletteView::paletteChanged, this, &PaletteView::updateText);
    updateText();

    //select color 0
    emit signalMapper->mappedInt(0);
}

void PaletteView::setPalette(MSXPalette* sourcePal)
{
    if (myPal != nullptr) {
        disconnect(myPal, &MSXPalette::paletteChanged, this, &PaletteView::paletteChanged);
    }

    myPal = sourcePal;
    myOriginalPal.copyDataFrom(*sourcePal);
    connect(myPal, &MSXPalette::paletteChanged, this, &PaletteView::paletteChanged);
    emit paletteReplaced(sourcePal);
}

MSXPalette* PaletteView::getPalette()
{
    return myPal;
}

void PaletteView::syncToSource()
{
    myPal->syncToSource();
    myOriginalPal.copyDataFrom(*myPal);
}

void PaletteView::setAutoSync(bool value)
{
    if (autoSync == value) return;
    autoSync = value;
    ui->cb_autosync->setChecked(value);
    if (value) {
        syncToSource();
    }
}

QJsonObject PaletteView::save2json()
{
    QJsonObject obj;
    obj["viewtext"] = ui->cb_viewtext->isChecked();
    obj["autosync"] = ui->cb_autosync->isChecked();
    obj["palette"] = ui->cbPalette->currentIndex();
    return obj;
}

bool PaletteView::loadFromJson(const QJsonObject& obj)
{
    auto vt = obj["viewtext"];
    auto as = obj["autosync"];
    auto pa = obj["palette"];
    if (vt == QJsonValue::Undefined ||
        as == QJsonValue::Undefined ||
        pa == QJsonValue::Undefined) {
            return false;
    }

    ui->cb_viewtext->setChecked(vt.toBool());
    ui->cb_autosync->setChecked(as.toBool());
    ui->cbPalette->setCurrentIndex(pa.toInt());
    return true;
}

void PaletteView::refresh()
{
}

void PaletteView::colorSelected(int colorNumber)
{
    currentColor = colorNumber;
    QRgb c = myPal->color(colorNumber);
    ui->horizontalSlider_R->setValue(qRed  (c) >> 5);
    ui->horizontalSlider_G->setValue(qGreen(c) >> 5);
    ui->horizontalSlider_B->setValue(qBlue (c) >> 5);
    ui->label_colornr->setText(QString("Color %1").arg(colorNumber));
}

void PaletteView::on_horizontalSlider_R_valueChanged(int value)
{
    ui->label_R->setText(QString("R=%1").arg(value));
    combineRGB();
}

void PaletteView::on_horizontalSlider_G_valueChanged(int value)
{
    ui->label_G->setText(QString("G=%1").arg(value));
    combineRGB();
}

void PaletteView::on_horizontalSlider_B_valueChanged(int value)
{
    ui->label_B->setText(QString("B=%1").arg(value));
    combineRGB();
}

void PaletteView::restorePalette()
{
    ScopedAssign sa(isDisplayUpdating, true);
    myPal->copyDataFrom(myOriginalPal);
    colorSelected(currentColor);
}

void PaletteView::on_buttonBox_clicked(QAbstractButton* button)
{
    if (button == ui->buttonBox->button(QDialogButtonBox::Apply) ||
        button == ui->buttonBox->button(QDialogButtonBox::Ok)) {
        syncToSource();
    } else if (button == ui->buttonBox->button(QDialogButtonBox::Reset) ||
               button == ui->buttonBox->button(QDialogButtonBox::Cancel)) {
        restorePalette();
    }
}

void PaletteView::on_cb_autosync_stateChanged(int arg1)
{
    autoSync = arg1 != Qt::Unchecked;
    if (autoSync) {
        syncToSource();
    }
}

void PaletteView::on_cbPalette_currentIndexChanged(int index)
{
    ui->pbCopyPaletteVDP->setEnabled(index != 0);
    ScopedAssign sa(isDisplayUpdating, true);
    setPalette(VDPDataStore::instance().getPalette(index));
    colorSelected(currentColor);
}

void PaletteView::on_pbCopyPaletteVDP_clicked()
{
    ScopedAssign sa(isDisplayUpdating, true);
    myPal->copyDataFrom(*VDPDataStore::instance().getPalette(paletteVDP));
    emit myPal->paletteChanged();
}

void PaletteView::updateText()
{
    ui->plainTextEdit->clear();
    for (int i = 0; i < 4; ++i) {
        QString txt(" db ");
        for (int j = 0; j < 4; ++j) {
            QRgb c = myPal->color(j + 4 * i);
            txt.append(QString("%1,%2 ").arg(
                           hexValue((qRed  (c) >> 5) * 16 + (qBlue(c) >> 5), 2),
                           hexValue((qGreen(c) >> 5), 2)));
            if (j < 3) txt.append(',');
        }
        ui->plainTextEdit->appendPlainText(txt);
    }
}

void PaletteView::combineRGB()
{
    if (isDisplayUpdating) return;

    int r = ui->horizontalSlider_R->value() & 7;
    int g = ui->horizontalSlider_G->value() & 7;
    int b = ui->horizontalSlider_B->value() & 7;
    myPal->setColor(currentColor, r, g, b);
    emit paletteChanged();
    if (autoSync) {
        syncToSource();
    }
}
