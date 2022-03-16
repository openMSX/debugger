#include "SpriteViewer.h"
#include "ui_SpriteViewer.h"
#include "VDPDataStore.h"
#include "VramSpriteView.h"
#include "Convert.h"

SpriteViewer::SpriteViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpriteViewer)
{
    ui->setupUi(this);
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->plainTextEdit->setFont(fixedFont);

    //now hook up some signals and slots
    //this way we have created the VDPDataStore::instance before our imagewidget
    //this alows the VDPDatastore to start asking for data as quickly as possible
    connect(ui->refreshButton, SIGNAL(clicked(bool)),
            &VDPDataStore::instance(), SLOT(refresh()));
    connect(&VDPDataStore::instance(), SIGNAL(dataRefreshed()),
            this, SLOT(VDPDataStoreDataRefreshed()));

    imageWidget = new VramSpriteView();
    //QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
    //sizePolicy1.setHorizontalStretch(0);
    //sizePolicy1.setVerticalStretch(0);
    //sizePolicy1.setHeightForWidth(imageWidget->sizePolicy().hasHeightForWidth());
    //imageWidget->setSizePolicy(sizePolicy1);
    imageWidget->setMinimumSize(QSize(256, 212));
    connect(&VDPDataStore::instance(), SIGNAL(dataRefreshed()),
            imageWidget, SLOT(refresh()));
    ui->spritePatternGenerator_widget->parentWidget()->layout()->replaceWidget(ui->spritePatternGenerator_widget,imageWidget);

    imageWidget->setVramSource(VDPDataStore::instance().getVramPointer());
    imageWidget->setPaletteSource(VDPDataStore::instance().getPalettePointer());

    imageWidgetSingle = new VramSpriteView(nullptr,VramSpriteView::PatternMode,true);
    QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(imageWidgetSingle->sizePolicy().hasHeightForWidth());
    imageWidgetSingle->setSizePolicy(sizePolicy3);
    imageWidgetSingle->setMinimumSize(QSize(64, 64));
    connect(&VDPDataStore::instance(), SIGNAL(dataRefreshed()),
            imageWidgetSingle, SLOT(refresh()));
    ui->single_spritePatternGenerator_widget->parentWidget()->layout()->replaceWidget(ui->single_spritePatternGenerator_widget,imageWidgetSingle);

    imageWidgetSingle->setVramSource(VDPDataStore::instance().getVramPointer());
    imageWidgetSingle->setPaletteSource(VDPDataStore::instance().getPalettePointer());





    imageWidgetSpat = new VramSpriteView(nullptr,VramSpriteView::SpriteAttributeMode);
//    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    sizePolicy2.setHorizontalStretch(0);
//    sizePolicy2.setVerticalStretch(0);
//    sizePolicy2.setHeightForWidth(imageWidgetSpat->sizePolicy().hasHeightForWidth());
//    imageWidgetSpat->setSizePolicy(sizePolicy2);
    imageWidgetSpat->setMinimumSize(QSize(256, 212));
    connect(&VDPDataStore::instance(), SIGNAL(dataRefreshed()),
            imageWidgetSpat, SLOT(refresh()));
    ui->spriteAttributeTable_widget->parentWidget()->layout()->replaceWidget(ui->spriteAttributeTable_widget,imageWidgetSpat);

    imageWidgetSpat->setVramSource(VDPDataStore::instance().getVramPointer());
    imageWidgetSpat->setPaletteSource(VDPDataStore::instance().getPalettePointer());



    setCorrectVDPData();
    setCorrectEnabled(ui->useVDPRegisters->isChecked());

    connect(imageWidget,SIGNAL(imageClicked(int , int , int , QString )),
            this,SLOT(pgtwidget_mouseClickedEvent(int , int , int , QString)));
    connect(imageWidget,SIGNAL(imagePosition(int , int , int )),
            this,SLOT(pgtwidget_mouseMoveEvent(int , int , int )));


    connect(imageWidgetSpat,SIGNAL(imageClicked(int , int , int , QString )),
            this,SLOT(spatwidget_mouseClickedEvent(int , int , int , QString)));
    connect(imageWidgetSpat,SIGNAL(imagePosition(int , int , int )),
            this,SLOT(spatwidget_mouseMoveEvent(int , int , int )));




    connect(ui->cb_displaygrid,SIGNAL(stateChanged(int)),
            this,SLOT(setDrawGrid(int)));

    // and now go fetch the initial data
    VDPDataStore::instance().refresh();
}

SpriteViewer::~SpriteViewer()
{
    delete ui;
}

void SpriteViewer::refresh()
{
    // All of the code is in the VDPDataStore;
    VDPDataStore::instance().refresh();
}

void SpriteViewer::VDPDataStoreDataRefreshed()
{
    if (ui->useVDPRegisters->isChecked()){
        decodeVDPregs();
        setCorrectVDPData();
    }
}

void SpriteViewer::pgtwidget_mouseMoveEvent(int x, int y, int character)
{
    ui->label_spgt_pat->setText(QString::number(character));
    ui->label_spg_adr->setText(hexValue(pgtaddr+ character*8,4));
    ui->label_spg_inspat->setText(QString("maybe..."));

    imageWidgetSingle->setCharToDisplay(character);
}

void SpriteViewer::pgtwidget_mouseClickedEvent(int x, int y, int character, QString text)
{
    pgtwidget_mouseMoveEvent(x,y,character);

    ui->plainTextEdit->setPlainText(QString("info for pattern %1 (%2)\n%3")
                                    .arg(character)
                                    .arg(hexValue(character,2))
                                    .arg(text)
                );
}

void SpriteViewer::spatwidget_mouseMoveEvent(int x, int y, int character)
{
    ui->label_spat_sprite->setText(QString::number(character));
    ui->label_spat_adr->setText(hexValue(spataddr+4*character,4));
    const unsigned char* vramBase = VDPDataStore::instance().getVramPointer();
    if (vramBase != nullptr && character<32){
        ui->label_spat_posx->setText(QString::number(vramBase[spataddr+4*character]));
        ui->label_spat_posy->setText(QString::number(vramBase[spataddr+4*character+1]));
        ui->label_spat_pattern->setText(QString::number(vramBase[spataddr+4*character+2]));
        ui->label_spat_color->setText(QString::number(vramBase[spataddr+4*character+3]));
    } else {
        ui->label_spat_posx->setText("");
        ui->label_spat_posy->setText("");
        ui->label_spat_pattern->setText("");
        ui->label_spat_color->setText("");
    };
}

void SpriteViewer::spatwidget_mouseClickedEvent(int x, int y, int character, QString text)
{
    spatwidget_mouseMoveEvent(x,y,character);
    ui->plainTextEdit->setPlainText(QString("info for sprite %1 (%2)\n%3")
                                    .arg(character)
                                    .arg(hexValue(character,2))
                                    .arg(text)
                );
}

void SpriteViewer::setDrawGrid(int state)
{
    imageWidget->setDrawgrid(state==Qt::Checked);
    imageWidgetSingle->setDrawgrid(state==Qt::Checked);
    imageWidgetSpat->setDrawgrid(state==Qt::Checked);
}

void SpriteViewer::decodeVDPregs()
{
    const unsigned char* regs = VDPDataStore::instance().getRegsPointer();
    //first determine sprite mode from screenmode
    int v = ((regs[0] & 0x0E) << 1)  | ((regs[1] & 0x18) >> 3);
    unsigned char patadrmask=255;
    switch (v) {
    case 2: //Text1
    case 10: //Text2
        spritemode=0; //no sprites here!
        size16x16=false;
        magnified=false;
        spritesenabled=false;
        break;
    case 0: //Graphic1
    case 1: //MultiColor
    case 4: //Graphic2
        spritemode=1;
        spritesenabled=regs[8]&2?false:true;
        size16x16=regs[1]&2;
        magnified=regs[1]&1;
        break;
    default:
        spritemode=2;
    	patadrmask=0xfc;
        spritesenabled=regs[8]&2?false:true;
        size16x16=regs[1]&2;
        magnified=regs[1]&1;
    };

//    ui-> ->setEnabled(spritemode!=0);
    spataddr=(regs[11]<<15)+((regs[5]&patadrmask)<<7);
    pgtaddr=regs[6]<<11;
    spcoladdr=spataddr& 0xFFFFFDFF; //only used in spritemode2


}

void SpriteViewer::on_le_patterntable_textChanged(const QString &arg1)
{
    int i = stringToValue(arg1);
    if (i != -1){
        spataddr=i;
        imageWidget->setPatternTableAddress(i);
        imageWidgetSingle->setPatternTableAddress(i);
        imageWidgetSpat->setPatternTableAddress(i);
        auto font = ui->le_patterntable->font();
        font.setItalic(false);
        ui->le_patterntable->setFont(font);
    } else {
        auto font = ui->le_patterntable->font();
        font.setItalic(true);
        ui->le_patterntable->setFont(font);
    }
}

void SpriteViewer::on_le_attributentable_textChanged(const QString &arg1)
{
    int i = stringToValue(arg1);
    if (i != -1){
        imageWidget->setAttributeTableAddress(i);
        imageWidgetSingle->setAttributeTableAddress(i);
        imageWidgetSpat->setAttributeTableAddress(i);
        auto font = ui->le_attributentable->font();
        font.setItalic(false);
        ui->le_attributentable->setFont(font);
    } else {
        auto font = ui->le_attributentable->font();
        font.setItalic(true);
        ui->le_attributentable->setFont(font);
    }
}

void SpriteViewer::on_useVDPRegisters_toggled(bool checked)
{
    if (checked){
        decodeVDPregs();
        setCorrectVDPData();
    }
    setCorrectEnabled(checked);


}

void SpriteViewer::setCorrectEnabled(bool checked)
{
    ui->cb_spritemode->setEnabled(!checked);
    ui->cb_size->setEnabled(!checked && spritemode!=0);
    ui->cb_mag->setEnabled(!checked && spritemode!=0);

    ui->le_patterntable->setEnabled(!checked);
    ui->le_attributentable->setEnabled(!checked);
    ui->le_spat_colortable->setEnabled(!checked&&spritemode==2);
    ui->le_colortable->setEnabled(!checked&&spritemode==2);


    ui->label_size->setEnabled(spritemode!=0);
    ui->label_mag->setEnabled(spritemode!=0);
    ui->label_spritesenabled->setEnabled(spritesenabled);

    ui->gb_colpat->setVisible(spritemode==2);

}

void SpriteViewer::setCorrectVDPData()
{
    imageWidget->setSize16x16(size16x16);
    imageWidget->setSpritemode(spritemode);

    imageWidgetSingle->setSize16x16(size16x16);
    imageWidgetSingle->setSpritemode(spritemode);


    imageWidgetSpat->setSize16x16(size16x16);
    imageWidgetSpat->setSpritemode(spritemode);



    ui->cb_spritemode->setCurrentIndex(spritemode);
    ui->cb_size->setCurrentIndex(size16x16?1:0);
    ui->cb_mag->setCurrentIndex(magnified?1:0);

    //we write to the lineedits and the textchanged slots  will write the values to the imagewidgets...
    ui->le_attributentable->setText(hexValue(spataddr,4));
    ui->le_patterntable->setText(hexValue(pgtaddr,4));
    ui->le_colortable->setText(hexValue(spcoladdr,4));

    ui->label_spgt_size->setText(size16x16?QString("(16x16)"):QString("(8x8)"));

}

void SpriteViewer::on_cb_size_currentIndexChanged(int index)
{
    size16x16=(index==0?false:true);
    ui->label_spgt_size->setText(size16x16?QString("(16x16)"):QString("(8x8)"));
    imageWidget->setSize16x16(size16x16);
    imageWidgetSingle->setSize16x16(size16x16);
    imageWidgetSpat->setSize16x16(size16x16);
}

void SpriteViewer::on_cb_spritemode_currentIndexChanged(int index)
{
    ui->gb_colpat->setVisible(index==2);
    imageWidget->setSpritemode(index);
    imageWidgetSingle->setSpritemode(index);
    imageWidgetSpat->setSpritemode(index);

}


void SpriteViewer::on_le_colortable_textChanged(const QString &arg1)
{
    int i = stringToValue(arg1);
    printf(" new   on_le_colortable_textChanged  %05x \n",i);
    if (i != -1){
        spcoladdr=i;
        imageWidget->setColorTableAddress(i);
        imageWidgetSingle->setColorTableAddress(i);
        imageWidgetSpat->setColorTableAddress(i);
        auto font = ui->le_patterntable->font();
        font.setItalic(false);
        ui->le_patterntable->setFont(font);
    } else {
        auto font = ui->le_patterntable->font();
        font.setItalic(true);
        ui->le_patterntable->setFont(font);
    }
}


void SpriteViewer::on_sp_zoom_valueChanged(int arg1)
{
    int zoom=arg1*2;
    imageWidget->setZoom(zoom);
    imageWidgetSpat->setZoom(zoom);
}


void SpriteViewer::on_cb_ecinfluence_toggled(bool checked)
{
    imageWidget->setECinfluence(checked);
    imageWidgetSpat->setECinfluence(checked);
}


void SpriteViewer::on_cb_mag_currentIndexChanged(int index)
{
    imageWidget->setUseMagnification(index==1);
    imageWidgetSpat->setUseMagnification(index==1);
}

