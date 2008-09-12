// $Id: $

#include "MainMemoryViewer.h"
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "CPURegs.h"
#include "CPURegsViewer.h"


const int MainMemoryViewer::linkRegisters[] = {
	CpuRegs::REG_BC, CpuRegs::REG_DE, CpuRegs::REG_HL,
	CpuRegs::REG_IX, CpuRegs::REG_IY,
	CpuRegs::REG_BC2, CpuRegs::REG_DE2, CpuRegs::REG_HL2,
	CpuRegs::REG_PC, CpuRegs::REG_SP } ;

MainMemoryViewer::MainMemoryViewer(QWidget* parent)
	: QWidget(parent)
{
	// create selection list, address edit line and viewer
        addressSourceList = new QComboBox;
        addressSourceList->setEditable(false);
        addressSourceList->addItem("Address:");
	for (int i=0 ; i < 10 ; i++){
		QString txt= QString("Linked to ");
		txt.append(CpuRegs::regNames[linkRegisters[i]]);
		addressSourceList->addItem(txt);
	};
	
	addressValue = new QLineEdit;
        addressValue->setText("0000");
        //addressValue->setEditable(false);

        hexView = new HexViewer;
	hexView->setUseMarker(true);
	hexView->setIsEditable(true);
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->setMargin(0);
        hbox->addWidget( addressSourceList );
        hbox->addWidget( addressValue );

        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->setMargin(0);
        vbox->addLayout( hbox );
        vbox->addWidget( hexView );

        setLayout( vbox );

	isLinked=false; 
	linkedId=0;
	regsViewer=NULL;

	connect( hexView, SIGNAL(locationChanged(int) ), this, SLOT(hexViewChanged(int) ) );
	connect( addressValue, SIGNAL(returnPressed() ), this, SLOT(addressValueChanged() ) );
	connect( addressSourceList, SIGNAL(currentIndexChanged(int) ), this, SLOT(addressSourceListChanged(int) ) );
}

MainMemoryViewer::~MainMemoryViewer()
{
}

void MainMemoryViewer::settingsChanged()
{
	hexView->settingsChanged();
}

void MainMemoryViewer::setLocation(int addr)
{
	addressValue->setText(QString().sprintf("%04X",addr));
	hexView->setLocation(addr);
}


void MainMemoryViewer::setDebuggable( const QString& name, int size )
{
	hexView->setDebuggable(name,size);
}

void MainMemoryViewer::setRegsView( CPURegsViewer* viewer )
{
	regsViewer=viewer;
}

void MainMemoryViewer::refresh()
{
	hexView->refresh();
}
void MainMemoryViewer::hexViewChanged(int addr)
{
	addressValue->setText(QString().sprintf("%04X",addr));
}
void MainMemoryViewer::addressValueChanged()
{
	hexView->setLocation(addressValue->text().toInt(NULL,16));
}
void MainMemoryViewer::registerChanged(int id, int value)
{
	if (!isLinked || (id != linkedId) ) {
		return;
	};
	
	addressValue->setText(QString().sprintf("%04X",value));
	hexView->setLocation(value);
	//hexView->refresh();
}
void MainMemoryViewer::addressSourceListChanged(int index)
{
	if ( index == 0 ){
		isLinked = false;
		addressValue->setReadOnly(false);
		hexView->setEnabledScrollBar(true);
	} else {
		isLinked = true;
		linkedId = linkRegisters[ index -1 ];
		addressValue->setReadOnly(true);
		hexView->setEnabledScrollBar(false);
		if (regsViewer){
			setLocation(regsViewer->readRegister(linkedId));
		}		
	}
}
