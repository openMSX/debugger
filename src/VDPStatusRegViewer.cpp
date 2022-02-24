#include "VDPStatusRegViewer.h"
#include "VramBitMappedView.h"
#include "VDPDataStore.h"
#include "InteractiveLabel.h"
#include <QMessageBox>
#include <QPalette>


highlightDispatcher::highlightDispatcher()
{
	counter = 0;
}

void highlightDispatcher::receiveState(bool state)
{
	if (state) {
		if (counter == 0) {
			emit stateDispatched(true);
		}
		++counter;
	} else {
		--counter;
		if (counter == 0) {
			emit stateDispatched(false);
		}
	}
}


VDPStatusRegViewer::VDPStatusRegViewer(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	//statusregs =  VDPDataStore::instance().getStatusRegsPointer();
	statusregs = new unsigned char[16];

	// now hook up some signals and slots
	connectHighLights();

	// get initiale data
	refresh();
}

VDPStatusRegViewer::~VDPStatusRegViewer()
{
	delete[] statusregs;
}

void VDPStatusRegViewer::decodeVDPStatusRegs()
{
	// first update all hex values
	label_val_0->setText(QString("%1").arg(statusregs[0],2,16,QChar('0')).toUpper());
	label_val_1->setText(QString("%1").arg(statusregs[1],2,16,QChar('0')).toUpper());
	label_val_2->setText(QString("%1").arg(statusregs[2],2,16,QChar('0')).toUpper());
	label_val_3->setText(QString("%1").arg(statusregs[3],2,16,QChar('0')).toUpper());
	label_val_4->setText(QString("%1").arg(statusregs[4],2,16,QChar('0')).toUpper());
	label_val_5->setText(QString("%1").arg(statusregs[5],2,16,QChar('0')).toUpper());
	label_val_6->setText(QString("%1").arg(statusregs[6],2,16,QChar('0')).toUpper());
	label_val_7->setText(QString("%1").arg(statusregs[7],2,16,QChar('0')).toUpper());
	label_val_8->setText(QString("%1").arg(statusregs[8],2,16,QChar('0')).toUpper());
	label_val_9->setText(QString("%1").arg(statusregs[9],2,16,QChar('0')).toUpper());

	// update all the individual bits
	for (int r = 0; r <= 9; ++r) {
		for (int b = 7; b >= 0; --b) {
			QString name = QString("label_%1_%2").arg(r).arg(b);
			auto* l = findChild<InteractiveLabel*>(name);
			l->setText((statusregs[r] & (1 << b)) ? "1" : "0");
		}
	}

	// Start the interpretation
	label_I_0_7->setText((statusregs[0] & 128) ? "Interrupt" : "No int");
	label_I_0_6->setText((statusregs[0] &  64) ? "5th sprite" : "No 5th");
	label_I_0_5->setText((statusregs[0] &  32) ? "Collision" : "No collision");
	label_I_0_0->setText(QString("sprnr:%1").arg(statusregs[0] & 31));

	label_I_1_7->setText((statusregs[1] & 128) ? "Light" : "No light");
	label_I_1_6->setText((statusregs[1] &  64) ? "switch on" : "Switch off");
	label_I_1_0->setText((statusregs[1] &   1) ? "hor scanline int" : "no hor int");
	QString id;
	switch (statusregs[1] & 62) {
	case 0:
		id = QString("v9938");
		break;
	case 2:
		id = QString("v9948");
		break;
	case 4:
		id = QString("v9958");
		break;
	default:
		id = QString("unknown VDP");
	}
	label_I_1_1->setText(id);

	label_I_2_7->setText((statusregs[2] & 128) ? "Transfer ready" : "Transferring");
	label_I_2_6->setText((statusregs[2] &  64) ? "Vertical scanning" : "Not vert scan");
	label_I_2_5->setText((statusregs[2] &  32) ? "Horizontal scanning" : "Not hor scan");
	label_I_2_4->setText((statusregs[2] &  16) ? "Boundary color detected" : "BC not deteced");
	label_I_2_1->setText((statusregs[2] & 2) ? "First field" : "Second field");
	label_I_2_0->setText((statusregs[2] & 1) ? "Command execution" : "No command exec");

	label_I_3->setText(QString("Column: %1").arg(statusregs[3] | ((statusregs[4] & 1) << 8)));
	label_I_5->setText(QString("Row: %1").arg(statusregs[5] | ((statusregs[6] & 3) << 8)));
	label_I_7->setText(QString("Color: %1").arg(statusregs[7]));
	label_I_8->setText(QString("Border X: %1").arg(statusregs[8] | ((statusregs[9] & 1) << 8)));
}

void VDPStatusRegViewer::doConnect(InteractiveLabel* lab, highlightDispatcher* dis)
{
	connect(lab, SIGNAL(mouseOver(bool)),     dis, SLOT(receiveState(bool)));
	connect(dis, SIGNAL(stateDispatched(bool)), lab, SLOT(highlight(bool)));
}

void VDPStatusRegViewer::makeGroup(QList<InteractiveLabel*> list, InteractiveLabel* explained)
{
	// First "steal" the Tooltip of the explained widget.
	for (auto* item : list) {
		item->setToolTip(explained->toolTip());
	}
	// now create a dispatcher and connect all to them
	list << explained;
	auto* dispat = new highlightDispatcher();
	for (auto* item : list) {
		doConnect(item, dispat);
	}
}

void VDPStatusRegViewer::connectHighLights()
{
	QList<InteractiveLabel*> list;

	list.clear();
	list << label_0_7;
	makeGroup(list, label_I_0_7);

	list.clear();
	list << label_0_6;
	makeGroup(list, label_I_0_6);

	list.clear();
	list << label_0_5;
	makeGroup(list, label_I_0_5);

	list.clear();
	list << label_0_4 << label_0_3 << label_0_2 << label_0_1 << label_0_0;
	makeGroup(list, label_I_0_0);

	list.clear();
	list << label_1_7;
	makeGroup(list, label_I_1_7);

	list.clear();
	list << label_1_6;
	makeGroup(list, label_I_1_6);

	list.clear();
	list << label_1_4 << label_1_3 << label_1_2 << label_1_1;
	makeGroup(list, label_I_1_1);

	list.clear();
	list << label_1_0;
	makeGroup(list, label_I_1_0);

	list.clear();
	list << label_2_7;
	makeGroup(list, label_I_2_7);

	list.clear();
	list << label_2_6;
	makeGroup(list, label_I_2_6);

	list.clear();
	list << label_2_5;
	makeGroup(list, label_I_2_5);

	list.clear();
	list << label_2_4;
	makeGroup(list, label_I_2_4);

	list.clear();
	list << label_2_1;
	makeGroup(list, label_I_2_1);

	list.clear();
	list << label_2_0;
	makeGroup(list, label_I_2_0);

	list.clear();
	list << label_2_0;
	makeGroup(list, label_I_2_0);

	list.clear();
	list << label_3_7 << label_3_6 << label_3_5 << label_3_4;
	list << label_3_3 << label_3_2 << label_3_1 << label_3_0;
	list << label_4_7 << label_4_6 << label_4_5 << label_4_4;
	list << label_4_3 << label_4_2 << label_4_1 << label_4_0;
	makeGroup(list, label_I_3);

	list.clear();
	list << label_5_7 << label_5_6 << label_5_5 << label_5_4;
	list << label_5_3 << label_5_2 << label_5_1 << label_5_0;
	list << label_6_7 << label_6_6 << label_6_5 << label_6_4;
	list << label_6_3 << label_6_2 << label_6_1 << label_6_0;
	makeGroup(list, label_I_5);

	list.clear();
	list << label_7_7 << label_7_6 << label_7_5 << label_7_4;
	list << label_7_3 << label_7_2 << label_7_1 << label_7_0;
	makeGroup(list, label_I_7);

	list.clear();
	list << label_8_7 << label_8_6 << label_8_5 << label_8_4;
	list << label_8_3 << label_8_2 << label_8_1 << label_8_0;
	list << label_9_7 << label_9_6 << label_9_5 << label_9_4;
	list << label_9_3 << label_9_2 << label_9_1 << label_9_0;
	makeGroup(list, label_I_8);
}

void VDPStatusRegViewer::refresh()
{
	new SimpleHexRequest("{VDP status regs}", 0, 16, statusregs, *this);
}

void VDPStatusRegViewer::DataHexRequestReceived()
{
	decodeVDPStatusRegs();
}
