#include "VDPRegViewer.h"
#include "VDPDataStore.h"
#include <QPalette>
#include "InteractiveButton.h"


buttonHighlightDispatcher::buttonHighlightDispatcher() 
{
	counter=0;
}


void buttonHighlightDispatcher::receiveState(bool state)
{
	if (counter==0 && state){
		emit dispatchState(true);
	} else if (!state && counter== 1){
		emit dispatchState(false);
	}
	counter+= state?1:-1;
}



VDPRegViewer::VDPRegViewer( QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	regs =  VDPDataStore::instance().getRegsPointer();

	//now hook up some signals and slots
	connectHighLights();

	connect( &VDPDataStore::instance(), SIGNAL( dataRefreshed() ), this, SLOT( on_VDPDataStore_dataRefreshed() ) );
	//connect( &VDPDataStore::instance(), SIGNAL( dataRefreshed() ), imageWidget, SLOT( refresh() ) );
	//connect( refreshButton,  SIGNAL( clicked (bool) ), &VDPDataStore::instance(), SLOT( refresh() ) );

	connect( pushButton_0_7, SIGNAL( mouseOver(bool) ), pushButton_0_7, SLOT( highlight(bool) ) );
	connect( pushButton_0_6, SIGNAL( mouseOver(bool) ), pushButton_0_6, SLOT( highlight(bool) ) );
	connect( pushButton_0_5, SIGNAL( mouseOver(bool) ), pushButton_0_5, SLOT( highlight(bool) ) );
	connect( pushButton_0_4, SIGNAL( mouseOver(bool) ), pushButton_0_4, SLOT( highlight(bool) ) );
	connect( pushButton_0_3, SIGNAL( mouseOver(bool) ), pushButton_0_3, SLOT( highlight(bool) ) );

	// and now go fetch the initial data
	VDPDataStore::instance().refresh();
}

VDPRegViewer::~VDPRegViewer()
{
}

void VDPRegViewer::decodeVDPRegs()
{
/*
	//first update all hex values
	label_val_0->setText(QString("%1").arg(statusregs[0],2,16,QChar('0')));
	label_val_1->setText(QString("%1").arg(statusregs[1],2,16,QChar('0')));
	label_val_2->setText(QString("%1").arg(statusregs[2],2,16,QChar('0')));
	label_val_3->setText(QString("%1").arg(statusregs[3],2,16,QChar('0')));
	label_val_4->setText(QString("%1").arg(statusregs[4],2,16,QChar('0')));
	label_val_5->setText(QString("%1").arg(statusregs[5],2,16,QChar('0')));
	label_val_6->setText(QString("%1").arg(statusregs[6],2,16,QChar('0')));
	label_val_7->setText(QString("%1").arg(statusregs[7],2,16,QChar('0')));
	label_val_8->setText(QString("%1").arg(statusregs[8],2,16,QChar('0')));
	label_val_9->setText(QString("%1").arg(statusregs[9],2,16,QChar('0')));
	//update all the individual bits
	for (int r=0;r<=9;r++){
		for (int b=7;b>=0;b--){
			QString name=QString("label_%1_%2").arg(r).arg(b);
			InteractiveLabel *l = qFindChild<InteractiveLabel*>(this, name);
			l->setText((statusregs[r]&(1<<b))?"1":"0");
		};
	};
	

	// Start the interpretation
	label_I_0_7->setText((statusregs[0] & 128)?"Interrupt":"No int" );
	label_I_0_6->setText((statusregs[0] & 64)?"5th sprite":"No 5th" );
	label_I_0_5->setText((statusregs[0] & 32)?"Collision":"No collision" );
	label_I_0_0->setText(QString("sprnr:%1").arg(statusregs[0] & 31));

	label_I_1_7->setText((statusregs[1] & 128)?"Light":"No light" );
	label_I_1_6->setText((statusregs[1] & 64)?"switch on":"Switch off" );
	label_I_1_0->setText((statusregs[1] & 1)?"hor scanline int":"no hor int" );
	QString id;
	switch (statusregs[1] & 62){
		case 0:
			id=QString("v9938");
			break;;
		case 2:
			id=QString("v9948");
			break;;
		case 4:
			id=QString("v9958");
			break;;
		default:
			id=QString("unknown VDP");
	}
	label_I_1_1->setText( id );

	label_I_2_7->setText((statusregs[2] & 128)?"Transfer ready":"Transfering" );
	label_I_2_6->setText((statusregs[2] & 64)?"Vertical scanning":"Not vert scan" );
	label_I_2_5->setText((statusregs[2] & 32)?"Horizontal scanning":"Not hor scan" );
	label_I_2_4->setText((statusregs[2] & 16)?"Boundary color detected":"BC not deteced" );
	label_I_2_1->setText((statusregs[2] & 2)?"First field":"Second field" );
	label_I_2_0->setText((statusregs[2] & 2)?"Command execution":"No command exec" );

	label_I_3->setText(QString("Column: %1").arg(statusregs[3]|((statusregs[4]&1)<<8)));
	label_I_5->setText(QString("Row: %1").arg(statusregs[5]|((statusregs[6]&3)<<8)));
	label_I_7->setText(QString("Color: %1").arg(statusregs[7]));
	label_I_8->setText(QString("Border X: %1").arg(statusregs[8]|((statusregs[9]&1)<<8)));
*/
}

void VDPRegViewer::doConnect( InteractiveButton* but, buttonHighlightDispatcher* dis)
{
		connect( but, SIGNAL( mouseOver(bool) ), dis, SLOT( receiveState(bool) ) );
		connect( dis, SIGNAL( dispatchState(bool) ), but, SLOT( highlight(bool) ) );
};

void VDPRegViewer::connectHighLights()
{
	buttonHighlightDispatcher* dispat = new buttonHighlightDispatcher();
/*
	doConnect( label_0_7, dispat );
	doConnect( label_I_0_7, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_0_6, dispat );
	doConnect( label_I_0_6, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_0_5, dispat );
	doConnect( label_I_0_5, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_0_4, dispat );
	doConnect( label_0_3, dispat );
	doConnect( label_0_2, dispat );
	doConnect( label_0_1, dispat );
	doConnect( label_0_0, dispat );
	doConnect( label_I_0_0, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_1_7, dispat );
	doConnect( label_I_1_7, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_1_6, dispat );
	doConnect( label_I_1_6, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_1_5, dispat );
	doConnect( label_1_4, dispat );
	doConnect( label_1_3, dispat );
	doConnect( label_1_2, dispat );
	doConnect( label_1_1, dispat );
	doConnect( label_I_1_1, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_1_0, dispat );
	doConnect( label_I_1_0, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_7, dispat );
	doConnect( label_I_2_7, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_6, dispat );
	doConnect( label_I_2_6, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_5, dispat );
	doConnect( label_I_2_5, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_4, dispat );
	doConnect( label_I_2_4, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_1, dispat );
	doConnect( label_I_2_1, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_2_0, dispat );
	doConnect( label_I_2_0, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_3_7, dispat );
	doConnect( label_3_6, dispat );
	doConnect( label_3_5, dispat );
	doConnect( label_3_4, dispat );
	doConnect( label_3_3, dispat );
	doConnect( label_3_2, dispat );
	doConnect( label_3_1, dispat );
	doConnect( label_3_0, dispat );
	doConnect( label_4_7, dispat );
	doConnect( label_4_6, dispat );
	doConnect( label_4_5, dispat );
	doConnect( label_4_4, dispat );
	doConnect( label_4_3, dispat );
	doConnect( label_4_2, dispat );
	doConnect( label_4_1, dispat );
	doConnect( label_4_0, dispat );
	doConnect( label_I_3, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_5_7, dispat );
	doConnect( label_5_6, dispat );
	doConnect( label_5_5, dispat );
	doConnect( label_5_4, dispat );
	doConnect( label_5_3, dispat );
	doConnect( label_5_2, dispat );
	doConnect( label_5_1, dispat );
	doConnect( label_5_0, dispat );
	doConnect( label_6_7, dispat );
	doConnect( label_6_6, dispat );
	doConnect( label_6_5, dispat );
	doConnect( label_6_4, dispat );
	doConnect( label_6_3, dispat );
	doConnect( label_6_2, dispat );
	doConnect( label_6_1, dispat );
	doConnect( label_6_0, dispat );
	doConnect( label_I_5, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_7_7, dispat );
	doConnect( label_7_6, dispat );
	doConnect( label_7_5, dispat );
	doConnect( label_7_4, dispat );
	doConnect( label_7_3, dispat );
	doConnect( label_7_2, dispat );
	doConnect( label_7_1, dispat );
	doConnect( label_7_0, dispat );
	doConnect( label_I_7, dispat );

	dispat = new buttonHighlightDispatcher();
	doConnect( label_8_7, dispat );
	doConnect( label_8_6, dispat );
	doConnect( label_8_5, dispat );
	doConnect( label_8_4, dispat );
	doConnect( label_8_3, dispat );
	doConnect( label_8_2, dispat );
	doConnect( label_8_1, dispat );
	doConnect( label_8_0, dispat );
	doConnect( label_9_7, dispat );
	doConnect( label_9_6, dispat );
	doConnect( label_9_5, dispat );
	doConnect( label_9_4, dispat );
	doConnect( label_9_3, dispat );
	doConnect( label_9_2, dispat );
	doConnect( label_9_1, dispat );
	doConnect( label_9_0, dispat );
	doConnect( label_I_8, dispat );
*/

}



void VDPRegViewer::refresh()
{
	VDPDataStore::instance().refresh();
}




void VDPRegViewer::on_VDPDataStore_dataRefreshed()
{
	decodeVDPRegs();
}
