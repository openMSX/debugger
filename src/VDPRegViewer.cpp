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
	regs =  new unsigned char[64];

	//now hook up some signals and slots
	connectHighLights();

	//get initiale data
	refresh();

}

VDPRegViewer::~VDPRegViewer()
{
	delete[] regs;
}

void VDPRegViewer::decodeVDPRegs()
{
	//first update all hex values
	label_val_0->setText(QString("%1").arg(regs[0],2,16,QChar('0')).toUpper());
	label_val_1->setText(QString("%1").arg(regs[1],2,16,QChar('0')).toUpper());
	label_val_2->setText(QString("%1").arg(regs[2],2,16,QChar('0')).toUpper());
	label_val_3->setText(QString("%1").arg(regs[3],2,16,QChar('0')).toUpper());
	label_val_4->setText(QString("%1").arg(regs[4],2,16,QChar('0')).toUpper());
	label_val_5->setText(QString("%1").arg(regs[5],2,16,QChar('0')).toUpper());
	label_val_6->setText(QString("%1").arg(regs[6],2,16,QChar('0')).toUpper());
	label_val_7->setText(QString("%1").arg(regs[7],2,16,QChar('0')).toUpper());
	label_val_8->setText(QString("%1").arg(regs[8],2,16,QChar('0')).toUpper());
	label_val_9->setText(QString("%1").arg(regs[9],2,16,QChar('0')).toUpper());
	label_val_10->setText(QString("%1").arg(regs[10],2,16,QChar('0')).toUpper());
	label_val_11->setText(QString("%1").arg(regs[11],2,16,QChar('0')).toUpper());
	label_val_12->setText(QString("%1").arg(regs[12],2,16,QChar('0')).toUpper());
	label_val_13->setText(QString("%1").arg(regs[13],2,16,QChar('0')).toUpper());
	label_val_14->setText(QString("%1").arg(regs[14],2,16,QChar('0')).toUpper());
	label_val_15->setText(QString("%1").arg(regs[15],2,16,QChar('0')).toUpper());
	label_val_16->setText(QString("%1").arg(regs[16],2,16,QChar('0')).toUpper());
	label_val_17->setText(QString("%1").arg(regs[17],2,16,QChar('0')).toUpper());
	label_val_18->setText(QString("%1").arg(regs[18],2,16,QChar('0')).toUpper());
	label_val_19->setText(QString("%1").arg(regs[19],2,16,QChar('0')).toUpper());
	label_val_20->setText(QString("%1").arg(regs[20],2,16,QChar('0')).toUpper());
	label_val_21->setText(QString("%1").arg(regs[21],2,16,QChar('0')).toUpper());
	label_val_22->setText(QString("%1").arg(regs[22],2,16,QChar('0')).toUpper());
	label_val_23->setText(QString("%1").arg(regs[23],2,16,QChar('0')).toUpper());
	//update all the individual bits
	for (int r=0;r<=23;r++){
		for (int b=7;b>=0;b--){
			QString name=QString("pushButton_%1_%2").arg(r).arg(b);
			InteractiveButton *I = qFindChild<InteractiveButton*>(this, name);
			I->setChecked((regs[r]&(1<<b))?true:false);
		};
	};
	

	// Start the interpretation
	label_dec_ie2->setText((regs[0] & 32)?"Interrupt from lightpen enabled":"Interrupt from lightpen disabled" );
	label_dec_ie1->setText((regs[0] & 16)?"Horizontal scanning line int enabled":"Horizontal scanning line int disabled" );

	int m=((regs[0] & 14)<<1) | ((regs[1] & 24)>>3);
	const char* screen[]={"let's find out 0", "let's find out 1", "let's find out 2",
	  "let's find out 3", "let's find out 4", "let's find out 5",
	  "let's find out 6", "let's find out 7", "let's find out 8",
	  "let's find out 9", "let's find out 10", "let's find out 11",
	  "let's find out 12", "let's find out 13", "let's find out 14",
	  "let's find out 15", "let's find out 16", "let's find out 17",
	  "let's find out 18", "let's find out 19", "let's find out 20",
	  "let's find out 21", "let's find out 22", "let's find out 23",
	  "let's find out 24", "let's find out 25", "let's find out 26",
	  "let's find out 27", "let's find out 28", "let's find out 29",
	  "let's find out 30", "let's find out 31"};
	label_dec_m->setText(QString("Screen modebits %1 : SCREEN %2").arg(m).arg(screen[m]) );

	label_dec_bl->setText((regs[1] & 64)?"Display enabled":"Display disabled");
	label_dec_ie0->setText((regs[1] & 32)?"Horizontal scanning line int enabled":"Horizontal scanning line int disabled" );

	label_dec_si->setText((regs[1] & 2)?"16x16 sprites":"8x8 sprites" );
	label_dec_mag->setText((regs[1] & 1)?"magnified sprites":"regular sized" );

	label_dec_tp->setText((regs[8] & 32)? "Color 0 uses the color registers":"Color 0 is transparent (=shows border)"
 );
	label_dec_spd->setText((regs[8] & 2)?"Sprites enabled":"Sprites disabled");

	label_dec_ln->setText((regs[9] & 128)?"212":"192");
	label_dec_il->setText((regs[9] & 8)?"interlaced":"non-interlaced");
	label_dec_eo->setText((regs[9] & 4)?"alternate pages":"same page");
	label_dec_nt->setText((regs[9] & 2)?"PAL":"NTSC");

	//TODO mask according to screen mode, displayed values are wrong atm!!
	//TODO ignore bits if in MSX1 debug mode
	label_dec_r2->setText(QString("0x%1").arg(regs[2]<<10,5,16,QChar('0')).toUpper());
	label_dec_r3->setText(QString("0x%1").arg((regs[3]<<6)|(regs[10]<<14),5,16,QChar('0')).toUpper());
	label_dec_r4->setText(QString("0x%1").arg(regs[4]<<11,5,16,QChar('0')).toUpper());
	label_dec_r5->setText(QString("0x%1").arg((regs[5]<<7)|(regs[11]<<15),5,16,QChar('0')).toUpper());
	label_dec_r6->setText(QString("0x%1").arg(regs[6]<<11,5,16,QChar('0')).toUpper());

	label_dec_tc->setText(QString("%1").arg((regs[7]>>4)&15 ,2,10,QChar('0')));
	label_dec_bd->setText(QString("%1").arg( regs[7]&15     ,2,10,QChar('0')));
	label_dec_t2->setText(QString("%1").arg((regs[12]>>4)&15 ,2,10,QChar('0')));
	label_dec_bc->setText(QString("%1").arg( regs[12]&15     ,2,10,QChar('0')));
	label_dec_on->setText(QString("%1").arg((regs[13]>>4)&15 ,2,10,QChar('0')));
	label_dec_off->setText(QString("%1").arg( regs[13]&15     ,2,10,QChar('0')));

	int x=(regs[18]&15);
	int y=((regs[18]>>4)&15);
	x=x>7?16-x:0-x;
	y=y>7?16-y:0-y;
	label_dec_r18->setText(QString("(%1,%2)").arg(x).arg(y));

	label_dec_r19->setText(QString("%1").arg(regs[19] ,3,10));
	label_dec_r23->setText(QString("%1").arg(regs[23] ,3,10));

	label_dec_r14->setText(QString("0x%1").arg(regs[14]<<14,5,16,QChar('0')));
	label_dec_r15->setText(QString("%1").arg(regs[15]&15 ,2,10));
	label_dec_r16->setText(QString("%1").arg(regs[16]&15 ,2,10));
	label_dec_r17->setText(QString("%1").arg(regs[17]&63 ,3,10).append((regs[17]&128)?" ,auto incr":""));
}

void VDPRegViewer::doConnect( InteractiveButton* but, buttonHighlightDispatcher* dis)
{
		connect( but, SIGNAL( mouseOver(bool) ), dis, SLOT( receiveState(bool) ) );
		connect( dis, SIGNAL( dispatchState(bool) ), but, SLOT( highlight(bool) ) );
};


void VDPRegViewer::monoGroup( InteractiveButton* but, InteractiveLabel* lab)
{
	connect( lab, SIGNAL( mouseOver(bool) ), but, SLOT( highlight(bool) ) );
	connect( but, SIGNAL( mouseOver(bool) ), lab, SLOT( highlight(bool) ) );
	connect( lab, SIGNAL( mouseOver(bool) ), lab, SLOT( highlight(bool) ) );
}

void VDPRegViewer::makeGroup( QList<InteractiveButton*> list, InteractiveLabel* explained)
{
        // First "steal" the Tooltip of the explained widget.
        InteractiveButton* item;

	/*
        foreach (item , list){
                item->setToolTip( explained->toolTip() );
        };
	*/

        //Create a dispatcher and connect all to them
        buttonHighlightDispatcher* dispat = new buttonHighlightDispatcher();
	connect( explained, SIGNAL( mouseOver(bool) ), dispat, SLOT( receiveState(bool) ) );
	connect( dispat, SIGNAL( dispatchState(bool) ), explained, SLOT( highlight(bool) ) );
        foreach (item , list){
                doConnect( item, dispat );
        };
}



void VDPRegViewer::connectHighLights()
{
	QList<InteractiveButton*> list;

	// Before connecting the highlights we connect the special 
	// 'Toggled Bit' event
	// Warning: This function is not available with MSVC 6!! Not that it
	// matters to me on my Linux environment :-)
	list = this->findChildren<InteractiveButton*>();
        InteractiveButton* item;
        foreach (item , list){
		connect( item, SIGNAL( newBitValue(int,int,bool) ),
			 this, SLOT( registerBitChanged(int,int,bool) ) );
        };


	// register 0 (+M1,M2)
	monoGroup( pushButton_0_5,label_dec_ie2);
	monoGroup( pushButton_0_4,label_dec_ie1);

	list.clear();
	list << pushButton_0_3 << pushButton_0_2 << pushButton_0_1;
	list << pushButton_1_4 << pushButton_1_3;
	makeGroup( list, label_dec_m);

	// register 1
	monoGroup( pushButton_1_6,label_dec_bl);
	monoGroup( pushButton_1_5,label_dec_ie0);
	monoGroup( pushButton_1_1,label_dec_si);
	monoGroup( pushButton_1_0,label_dec_mag);

	// register 8
	//monoGroup( pushButton_8_7,label_dec_ms);
	//monoGroup( pushButton_8_6,label_dec_lp);
	monoGroup( pushButton_8_5,label_dec_tp);
	//monoGroup( pushButton_8_4,label_dec_cb);
	//monoGroup( pushButton_8_3,label_dec_vr);
	monoGroup( pushButton_8_1,label_dec_spd);
	//monoGroup( pushButton_8_0,label_dec_bw);

	// register 9
	monoGroup( pushButton_9_7,label_dec_ln);
	list.clear();
	list << pushButton_9_5 << pushButton_9_4 ;
	makeGroup( list, label_dec_m); //TODO fix label
	monoGroup( pushButton_9_3,label_dec_il);
	monoGroup( pushButton_9_2,label_dec_eo);
	monoGroup( pushButton_9_1,label_dec_nt);
	//monoGroup( pushButton_9_0,label_dec_dc);

	// register 2
	list.clear();
	list << pushButton_2_7 << pushButton_2_6 << pushButton_2_5;
	list << pushButton_2_4 << pushButton_2_3 << pushButton_2_2;
	list << pushButton_2_1 << pushButton_2_0;
	makeGroup( list, label_dec_r2);

	// register 3 + 10
	list.clear();
	list << pushButton_3_7 << pushButton_3_6 << pushButton_3_5;
	list << pushButton_3_4 << pushButton_3_3 << pushButton_3_2;
	list << pushButton_3_1 << pushButton_3_0;
	list << pushButton_10_7 << pushButton_10_6 << pushButton_10_5;
	list << pushButton_10_4 << pushButton_10_3 << pushButton_10_2;
	list << pushButton_10_1 << pushButton_10_0;
	makeGroup( list, label_dec_r3);

	// register 4
	list.clear();
	list << pushButton_4_7 << pushButton_4_6 << pushButton_4_5;
	list << pushButton_4_4 << pushButton_4_3 << pushButton_4_2;
	list << pushButton_4_1 << pushButton_4_0;
	makeGroup( list, label_dec_r4);

	// register 5 + 11
	list.clear();
	list << pushButton_5_7 << pushButton_5_6 << pushButton_5_5;
	list << pushButton_5_4 << pushButton_5_3 << pushButton_5_2;
	list << pushButton_5_1 << pushButton_5_0;
	list << pushButton_11_7 << pushButton_11_6 << pushButton_11_5;
	list << pushButton_11_4 << pushButton_11_3 << pushButton_11_2;
	list << pushButton_11_1 << pushButton_11_0;
	makeGroup( list, label_dec_r5);

	// register 6
	list.clear();
	list << pushButton_6_7 << pushButton_6_6 << pushButton_6_5;
	list << pushButton_6_4 << pushButton_6_3 << pushButton_6_2;
	list << pushButton_6_1 << pushButton_6_0;
	makeGroup( list, label_dec_r6);

	// register 7
	list.clear();
	list << pushButton_7_7 << pushButton_7_6;
	list << pushButton_7_5 << pushButton_7_4;
	makeGroup( list, label_dec_tc);

	list.clear();
	list << pushButton_7_3 << pushButton_7_2;
	list << pushButton_7_1 << pushButton_7_0;
	makeGroup( list, label_dec_bd);

	// register 12
	list.clear();
	list << pushButton_12_7 << pushButton_12_6;
	list << pushButton_12_5 << pushButton_12_4;
	makeGroup( list, label_dec_t2);

	list.clear();
	list << pushButton_12_3 << pushButton_12_2;
	list << pushButton_12_1 << pushButton_12_0;
	makeGroup( list, label_dec_bc);

	// register 13
	list.clear();
	list << pushButton_13_7 << pushButton_13_6;
	list << pushButton_13_5 << pushButton_13_4;
	makeGroup( list, label_dec_on);

	list.clear();
	list << pushButton_13_3 << pushButton_13_2;
	list << pushButton_13_1 << pushButton_13_0;
	makeGroup( list, label_dec_off);

	// register 14
	list.clear();
	list << pushButton_14_7 << pushButton_14_6 << pushButton_14_5;
	list << pushButton_14_4 << pushButton_14_3 << pushButton_14_2;
	list << pushButton_14_1 << pushButton_14_0;
	makeGroup( list, label_dec_r14);

	// register 15
	list.clear();
	list << pushButton_15_7 << pushButton_15_6 << pushButton_15_5;
	list << pushButton_15_4 << pushButton_15_3 << pushButton_15_2;
	list << pushButton_15_1 << pushButton_15_0;
	makeGroup( list, label_dec_r15);

	// register 16
	list.clear();
	list << pushButton_16_7 << pushButton_16_6 << pushButton_16_5;
	list << pushButton_16_4 << pushButton_16_3 << pushButton_16_2;
	list << pushButton_16_1 << pushButton_16_0;
	makeGroup( list, label_dec_r16);

	// register 17
	list.clear();
	list << pushButton_17_7 << pushButton_17_6 << pushButton_17_5;
	list << pushButton_17_4 << pushButton_17_3 << pushButton_17_2;
	list << pushButton_17_1 << pushButton_17_0;
	makeGroup( list, label_dec_r17);

	// register 18
	list.clear();
	list << pushButton_18_7 << pushButton_18_6 << pushButton_18_5;
	list << pushButton_18_4 << pushButton_18_3 << pushButton_18_2;
	list << pushButton_18_1 << pushButton_18_0;
	makeGroup( list, label_dec_r18);

	// register 19
	list.clear();
	list << pushButton_19_7 << pushButton_19_6 << pushButton_19_5;
	list << pushButton_19_4 << pushButton_19_3 << pushButton_19_2;
	list << pushButton_19_1 << pushButton_19_0;
	makeGroup( list, label_dec_r19);

	// register 23
	list.clear();
	list << pushButton_23_7 << pushButton_23_6 << pushButton_23_5;
	list << pushButton_23_4 << pushButton_23_3 << pushButton_23_2;
	list << pushButton_23_1 << pushButton_23_0;
	makeGroup( list, label_dec_r23);

}



void VDPRegViewer::refresh()
{
	new SimpleHexRequest("{VDP regs}",0,16,regs, *this);
}




void VDPRegViewer::DataHexRequestReceived()
{
	decodeVDPRegs();
}

void VDPRegViewer::registerBitChanged(int reg, int bit, bool state)
{
	//maybe this call is the result of our own SetChecked (VDPDataStorte
	//update event) 
	if (state == (regs[reg]&(1<<bit)) ) {
		return;
	};

	//state does not correspond to the current register setting so we send
	//an update command
	if (state){
		regs[reg] |= (1<<bit);
	} else {
		regs[reg] &= 255^(1<<bit);
	}
	CommClient::instance().sendCommand(
		new SimpleCommand(
			QString("debug write {VDP regs} %1 %2").arg(reg).arg(regs[reg]) 
		)
	);

	// Update display without waiting for the VDPDataStore update
	decodeVDPRegs();
	// and then we could request an update nevertheless since some other
	// objects might want to see this change through the VDPDataStore also
	// :-)
	// VDPDataStore::instance().refresh();

}
