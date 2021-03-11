#include <QSignalMapper>
#include <QPushButton>
#include <QGridLayout>
#include <QPainter>
#include "PaletteDialog.h"
#include "ui_PaletteDialog.h"
#include "Convert.h"


PalettePatch::PalettePatch(QWidget *parent, int palnr) : QPushButton(parent),mycolor(Qt::green),isSelected(false)
{
    msxpalnr=palnr;
}

//void PalettePatch::setColor(QRgb c)
//{
//    mycolor = c;
//    update();
//}

void PalettePatch::updatePaletteChanged(unsigned char *pal)
{
    int r = (pal[2 * msxpalnr + 0] & 0xf0) >> 4;
    int b = (pal[2 * msxpalnr + 0] & 0x0f);
    int g = (pal[2 * msxpalnr+ 1] & 0x0f);
    r = (r >> 1) | (r << 2) | (r << 5);
    b = (b >> 1) | (b << 2) | (b << 5);
    g = (g >> 1) | (g << 2) | (g << 5);

    mycolor = qRgb(r, g, b);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    update();
//    printf("PalettePatch::updatePaletteChanged %i\n",msxpalnr);
}

void PalettePatch::setHighlightTest(int colornr)
{
    if (isSelected == (colornr==msxpalnr)) return;

    isSelected=(colornr==msxpalnr);
    update();
}


void PalettePatch::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(this);
        painter.setPen(isSelected?(Qt::white):QColor(mycolor));
        painter.setBrush(QBrush(mycolor));
        painter.drawRect(0,0,this->width()-1,this->height()-1);
}



PaletteDialog::PaletteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PaletteDialog),sourcepal(nullptr),currentcolor(0),autoSync(false)
{
    ui->setupUi(this);
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->plainTextEdit->setFont(fixedFont);

    setWindowTitle("Palette editor");
    memset(mypal,0,32);
    memset(myoriginalpal,0,32);
    //add colors to colorframe

    signalMapper = new QSignalMapper(this);
    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->setSpacing(0);
    for (int i = 0; i < 16; ++i) {
            PalettePatch *button = new PalettePatch(nullptr,i);
            connect(button, SIGNAL(clicked(bool)), signalMapper,SLOT(map()));
            signalMapper->setMapping(button, i);
            gridLayout->addWidget(button, i/8, i%8);
            connect(this,SIGNAL(paletteChanged(unsigned char*) ),
                    button,SLOT(updatePaletteChanged(unsigned char*))
                    );
            connect(signalMapper, SIGNAL(mapped(int)),
                    button, SLOT(setHighlightTest(int))
                    );

        }

        connect(signalMapper, SIGNAL(mapped(int)),
                this, SLOT(colorSelected(int)));

        ui->colorsframe->setLayout(gridLayout);
        connect(this,SIGNAL(paletteChanged(unsigned char*)),
                this,SLOT(updateText()));
}

void PaletteDialog::updateText()
{
    ui->plainTextEdit->clear();
    for (int i=0; i<4 ;i++){
        QString txt(" db ");
        for (int j=0; j<4 ;j++){
            txt.append(QString("%1,%2 ")
                        .arg(hexValue(mypal[2*(j+4*i)],2))
                        .arg(hexValue(mypal[2*(j+4*i)+1],2))
                    );
            if (j<3){txt.append(',');};
        }
        ui->plainTextEdit->appendPlainText(txt);
    };
}

void PaletteDialog::restoreOpeningsPalette()
{
    memcpy(mypal,myoriginalpal,32);
    emit paletteChanged(mypal); //resets the PalettePatches
    colorSelected(currentcolor); //resets the sliders
    if (autoSync){
        syncToSource();
    }
}

PaletteDialog::~PaletteDialog()
{
    delete ui;
}

void PaletteDialog::colorSelected(int colornumber)
{
    currentcolor=colornumber;
    int r = (mypal[2 * currentcolor + 0] & 0xf0) >> 4;
    int b = (mypal[2 * currentcolor + 0] & 0x0f);
    int g = (mypal[2 * currentcolor + 1] & 0x0f);
    ui->horizontalSlider_R->setValue(r);
    ui->horizontalSlider_G->setValue(g);
    ui->horizontalSlider_B->setValue(b);
    ui->label_colornr->setText(QString("Color %1").arg(colornumber));
}

void PaletteDialog::setPalette(unsigned char *pal)
{
    sourcepal=pal;
    memcpy(myoriginalpal,pal,32);
    memcpy(mypal,pal,32);
    emit paletteChanged(mypal);
}

unsigned char* PaletteDialog::getPalette()
{
    return mypal;
}

//void PaletteDialog::decodepalette()
//{
//    for (int i = 0; i < 16; ++i) {
//        int r = (mypal[2 * i + 0] & 0xf0) >> 4;
//        int b = (mypal[2 * i + 0] & 0x0f);
//        int g = (mypal[2 * i + 1] & 0x0f);

//        r = (r >> 1) | (r << 2) | (r << 5);
//        b = (b >> 1) | (b << 2) | (b << 5);
//        g = (g >> 1) | (g << 2) | (g << 5);

//        msxpallet[i] = qRgb(r, g, b);

//        QGridLayout* l=dynamic_cast<QGridLayout*>(ui->colorsframe->layout());
//        dynamic_cast<QPushButton*>(l->itemAtPosition(i%8,i/8)->widget())->setText(QString("%1(%2%3%4)").arg(i).arg(r).arg(g).arg(b));

//    }
//}

void PaletteDialog::combineRGB()
{
    int r=ui->horizontalSlider_R->value();
    int g=ui->horizontalSlider_G->value();
    int b=ui->horizontalSlider_B->value();
    mypal[currentcolor*2]=r*16+b;
    mypal[currentcolor*2+1]=g;
    emit paletteChanged(mypal);
    if (autoSync){
        syncToSource();
    }
}

void PaletteDialog::syncToSource()
{
    memcpy(sourcepal,mypal,32);
    emit paletteSynced();
}

void PaletteDialog::setAutoSync(bool value)
{
    if (autoSync==value)return;

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


void PaletteDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (button== ui->buttonBox->button(QDialogButtonBox::Apply) ||
        button== ui->buttonBox->button(QDialogButtonBox::Ok)){
        syncToSource();
    } else
    if (button== ui->buttonBox->button(QDialogButtonBox::Reset) ||
        button== ui->buttonBox->button(QDialogButtonBox::Cancel)){
        restoreOpeningsPalette();
    }
}

void PaletteDialog::on_cb_autosync_stateChanged(int arg1)
{
    autoSync=(arg1 !=Qt::Unchecked);
    if (autoSync){
        syncToSource();
    };
}
