#include "VDPRegViewer.h"
#include "VDPDataStore.h"
#include "InteractiveButton.h"
#include "CommClient.h"

static const int VDP_TMS99X8 = 1;
static const int VDP_V9938 = 0;
static const int VDP_V9958 = 4;


// class buttonHighlightDispatcher

buttonHighlightDispatcher::buttonHighlightDispatcher()
	: counter(0)
{
}

void buttonHighlightDispatcher::receiveState(bool state)
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


// class VDPRegViewer

VDPRegViewer::VDPRegViewer(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	vdpId = 99; // make sure that we parse the first time the status registers are read
	vdpId = VDP_V9958; //quick hack for now

	//now hook up some signals and slots
	connectHighLights();

	//get initial data
	refresh();

	decodeStatusVDPRegs(); // part of the quick hack :-)
}

void VDPRegViewer::setRegisterVisible(int r, bool visible)
{
	QString name1 = QString("label_R%1").arg(r);
	auto* l1 = findChild<QLabel*>(name1);
	l1->setVisible(visible);

	QString name2 = QString("label_val_%1").arg(r);
	auto* l2 = findChild<QLabel*>(name2);
	l2->setVisible(visible);

	for (int b = 7; b >= 0; --b) {
		QString name3 = QString("pushButton_%1_%2").arg(r).arg(b);
		auto *i = findChild<InteractiveButton*>(name3);
		i->setVisible(visible);
	}

	// now hide/show the explications of the give register
	switch (r) {
	case 8:
		label_dec_tp->setVisible(visible);
		label_dec_spd->setVisible(visible);
		break;
	case 9:
		label_dec_nt->setVisible(visible);
		label_dec_nt2->setVisible(visible);
		label_dec_il->setVisible(visible);
		label_dec_eo->setVisible(visible);
		label_dec_ln->setVisible(visible);
		label_dec_ln2->setVisible(visible);
		break;
	case 12:
		label_t2->setVisible(visible);
		label_bc->setVisible(visible);
		label_dec_t2->setVisible(visible);
		label_dec_bc->setVisible(visible);
		break;
	case 13:
		label_on->setVisible(visible);
		label_off->setVisible(visible);
		label_dec_on->setVisible(visible);
		label_dec_off->setVisible(visible);
		break;
	}
}

void VDPRegViewer::decodeStatusVDPRegs()
{
	//const uint8_t* statusregs = regs + 64;
	//int id = statusregs[1] & 62; // + (machine_has_TMS99x8 ? 1 : 0)
	// test MSX1 id = 1;
	//if (vdpId != id) {
	//	vdpId = id;
		if (vdpId == VDP_TMS99X8) {
			// TMS9918 = MSX1 VDP
			groupBox_V9958->setVisible(false);
			groupBox_TableBase->setVisible(true);
			groupBox_Color->setVisible(true);
			groupBox_Display->setVisible(false);
			groupBox_Access->setVisible(false);
			groupBox_dec_V9958->setVisible(false);
			groupBox_dec_TableBase->setVisible(true);
			groupBox_dec_Color->setVisible(true);
			groupBox_dec_Display->setVisible(false);
			groupBox_dec_Access->setVisible(false);
			label_dec_416K->setVisible(true);
			label_dec_ie1->setVisible(false);
			label_dec_ie2->setVisible(false);
			label_dec_dg->setVisible(false);
			setRegisterVisible( 8, false);
			setRegisterVisible( 9, false);
			setRegisterVisible(10, false);
			setRegisterVisible(11, false);
			setRegisterVisible(12, false);
			setRegisterVisible(13, false);
			setRegisterVisible(20, false);
			setRegisterVisible(21, false);
			setRegisterVisible(22, false);
			pushButton_0_2->setText("0");
			pushButton_0_2->setToolTip("");
			pushButton_0_3->setText("0");
			pushButton_0_3->setToolTip("");
			pushButton_0_4->setText("0");
			pushButton_0_4->setToolTip("");
			pushButton_0_5->setText("0");
			pushButton_0_5->setToolTip("");
			pushButton_0_6->setText("0");
			pushButton_0_6->setToolTip("");
			pushButton_1_7->setText("4/16");
			pushButton_1_7->setToolTip(
				"4/16K selection\n"
				"0 selects 4027 RAM operation\n"
				"1 selects 4108/4116 RAM operation");
			// mask all A16 bits of regs < 7
			pushButton_2_6->setText("0");
			pushButton_4_5->setText("0");
			pushButton_6_5->setText("0");
			// mask all A15
			pushButton_2_5->setText("0");
			pushButton_4_4->setText("0");
			pushButton_6_4->setText("0");
			// mask all A14 bits of regs < 7
			pushButton_2_4->setText("0");
			pushButton_4_3->setText("0");
			pushButton_6_3->setText("0");
			disconnect(modeBitsDispat, nullptr, pushButton_0_2, nullptr);
			disconnect(modeBitsDispat, nullptr, pushButton_0_3, nullptr);
			disconnect(pushButton_0_2, nullptr, nullptr, nullptr);
			disconnect(pushButton_0_3, nullptr, nullptr, nullptr);
			disconnect(pushButton_0_4, nullptr, nullptr, nullptr);
			disconnect(pushButton_0_5, nullptr, nullptr, nullptr);
			disconnect(pushButton_0_6, nullptr, nullptr, nullptr);
			monoGroup(pushButton_1_7, label_dec_416K);
		} else {
			// V9938 = MSX2 VDP
			// or
			// V9958 = MSX2+ VDP
			groupBox_TableBase->setVisible(true);
			groupBox_Color->setVisible(true);
			groupBox_Display->setVisible(true);
			groupBox_Access->setVisible(true);
			groupBox_dec_TableBase->setVisible(true);
			groupBox_dec_Color->setVisible(true);
			groupBox_dec_Display->setVisible(true);
			groupBox_dec_Access->setVisible(true);
			label_dec_416K->setVisible(false);
			label_dec_ie1->setVisible(true);
			label_dec_dg->setVisible(true);
			setRegisterVisible( 8, true);
			setRegisterVisible( 9, true);
			setRegisterVisible(10, true);
			setRegisterVisible(11, true);
			setRegisterVisible(12, true);
			setRegisterVisible(13, true);
			setRegisterVisible(20, true);
			setRegisterVisible(21, true);
			setRegisterVisible(22, true);
			pushButton_0_2->setText("M4");
			pushButton_0_3->setText("M5");
			pushButton_0_4->setText("IE1");
			pushButton_0_6->setText("DG");
			pushButton_1_7->setText("0");
			pushButton_0_2->setToolTip("Used to change the display mode.");
			pushButton_0_3->setToolTip("Used to change the display mode.");
			pushButton_0_4->setToolTip("Enables interrupt from Horizontal scanning line by Interrupt Enable 1.");
			pushButton_0_6->setToolTip("Sets the color bus to input mode, and inputs data into the VRAM.");
			pushButton_1_7->setToolTip("");

			// all A16 bits of regs < 7
			pushButton_4_5->setText("A16");
			pushButton_6_5->setText("A16");
			// all A15
			pushButton_4_4->setText("A15");
			pushButton_6_4->setText("A15");
			// all A14
			pushButton_4_3->setText("A14");
			pushButton_6_3->setText("A14");
			////pushButton_0_5->setText("IE2");

			////disconnect(pushButton_0_7, 0, 0, 0);
			////disconnect(label_dec_416K, 0, 0, 0);

			////reGroup(pushButton_0_2, modeBitsDispat);
			////reGroup(pushButton_0_3, modeBitsDispat);

			////monoGroup(pushButton_0_4, label_dec_ie1);
			////monoGroup(pushButton_0_5, label_dec_ie2);
			////monoGroup(pushButton_0_6, label_dec_dg);

			//break;
			// V9958 = MSX2+ VDP
			////groupBox_TableBase->setVisible(true);
			////groupBox_Color->setVisible(true);
			////groupBox_Display->setVisible(true);
			////groupBox_Access->setVisible(true);
			////groupBox_dec_TableBase->setVisible(true);
			////groupBox_dec_Color->setVisible(true);
			////groupBox_dec_Display->setVisible(true);
			////groupBox_dec_Access->setVisible(true);
			////label_dec_416K->setVisible(false);
			////label_dec_ie1->setVisible(true);
			////label_dec_dg->setVisible(true);
			////setRegisterVisible( 8, true);
			////setRegisterVisible( 9, true);
			////setRegisterVisible(10, true);
			////setRegisterVisible(11, true);
			////setRegisterVisible(12, true);
			////setRegisterVisible(13, true);
			////setRegisterVisible(20, true);
			////setRegisterVisible(21, true);
			////setRegisterVisible(22, true);
			////pushButton_0_2->setText("M4");
			////pushButton_0_3->setText("M5");
			////pushButton_0_4->setText("IE1");
			////pushButton_0_6->setText("DG");
			////pushButton_1_7->setText("0");
			////pushButton_0_2->setToolTip("Used to change the display mode.");
			////pushButton_0_3->setToolTip("Used to change the display mode.");
			////pushButton_0_4->setToolTip("Enables interrupt from Horizontal scanning line by Interrupt Enable 1.");
			////pushButton_0_6->setToolTip("Sets the color bus to input mode, and inputs data into the VRAM.");
			////pushButton_1_7->setToolTip("");

			pushButton_1_7->setToolTip("");
			disconnect(pushButton_0_7, nullptr, nullptr, nullptr);
			disconnect(label_dec_416K, nullptr, nullptr, nullptr);

			reGroup(pushButton_0_2, modeBitsDispat);
			reGroup(pushButton_0_3, modeBitsDispat);

			monoGroup(pushButton_0_4, label_dec_ie1);
			monoGroup(pushButton_0_5, label_dec_ie2);
			monoGroup(pushButton_0_6, label_dec_dg);

			if (vdpId == VDP_V9938) {
				groupBox_V9958->setVisible(false);
				groupBox_dec_V9958->setVisible(false);
				label_dec_ie2->setVisible(true);
				pushButton_0_5->setText("IE2");
				pushButton_0_5->setToolTip("Enables interrupt from light pen by Interrupt Enable 2.");
				pushButton_8_6->setText("LP");
				pushButton_8_6->setToolTip("When 1, enables light pen. When 0, disables light pen.");
				pushButton_8_7->setText("MS");
				pushButton_8_7->setToolTip("When 1, sets the color bus to input mode and enables mouse. When 0, sets the color bus to output mode and disables mouse.");
			} else {
				groupBox_V9958->setVisible(true);
				groupBox_dec_V9958->setVisible(true);
				label_dec_ie2->setVisible(false);
				pushButton_0_5->setText("0");
				pushButton_0_5->setToolTip("");
				pushButton_8_7->setText("0");
				pushButton_8_7->setToolTip("");
				pushButton_8_6->setText("0");
				pushButton_8_6->setToolTip("");
				/* Since mouse/lightpen are disabled this will affect
				 * status reg1 bit 7 (LPS) and
				 * status reg1 bit 6 (FL)
				 */
			}
			//break;
		}
	//}
}

static QString dec2(int val)
{
	return QString("%1").arg(val, 2, 10, QChar('0'));
}
static QString dec3(int val)
{
	return QString("%1").arg(val, 3, 10, QChar('0'));
}
static QString hex2(int val)
{
	return QString("%1").arg(val, 2, 16, QChar('0')).toUpper();
}
static QString hex5(int val)
{
	return QString("%1").arg(val, 5, 16, QChar('0')).toUpper();
}
void VDPRegViewer::decodeVDPRegs()
{
	// first update all hex values
	label_val_0->setText(hex2(regs[0]));
	label_val_1->setText(hex2(regs[1]));
	label_val_2->setText(hex2(regs[2]));
	label_val_3->setText(hex2(regs[3]));
	label_val_4->setText(hex2(regs[4]));
	label_val_5->setText(hex2(regs[5]));
	label_val_6->setText(hex2(regs[6]));
	label_val_7->setText(hex2(regs[7]));

	// Only on V9938 and V9958 this makes sence
	if (vdpId != VDP_TMS99X8) {
		label_val_8 ->setText(hex2(regs[ 8]));
		label_val_9 ->setText(hex2(regs[ 9]));
		label_val_10->setText(hex2(regs[10]));
		label_val_11->setText(hex2(regs[11]));
		label_val_12->setText(hex2(regs[12]));
		label_val_13->setText(hex2(regs[13]));
		label_val_14->setText(hex2(regs[14]));
		label_val_15->setText(hex2(regs[15]));
		label_val_16->setText(hex2(regs[16]));
		label_val_17->setText(hex2(regs[17]));
		label_val_18->setText(hex2(regs[18]));
		label_val_19->setText(hex2(regs[19]));
		label_val_20->setText(hex2(regs[20]));
		label_val_21->setText(hex2(regs[21]));
		label_val_22->setText(hex2(regs[22]));
		label_val_23->setText(hex2(regs[23]));
	}
	// Only on V9958 this makes sence
	if (vdpId == VDP_V9958) {
		label_val_25->setText(hex2(regs[25]));
		label_val_26->setText(hex2(regs[26]));
		label_val_27->setText(hex2(regs[27]));
	}

	// determine screenmode
	int m = ((regs[0] & 0x0E) << 1) | ((regs[1] & 0x18) >> 3);
	const char* const screen[32] = {
		"SCREEN 1", "SCREEN 3", "SCREEN 0",
		"let's find out 3", "SCREEN 2", "let's find out 5",
		"let's find out 6", "let's find out 7", "SCREEN 4",
		"let's find out 9", "SCREEN 0 width 80", "let's find out 11",
		"SCREEN 5", "let's find out 13", "let's find out 14",
		"let's find out 15", "SCREEN 6", "let's find out 17",
		"let's find out 18", "let's find out 19", "SCREEN 7",
		"let's find out 21", "let's find out 22", "let's find out 23",
		"let's find out 24", "let's find out 25", "let's find out 26",
		"let's find out 27", "SCREEN 8", "let's find out 29",
		"let's find out 30", "let's find out 31"
	};
	int basicScreen = [&] {
		switch (m) {
			case  2: return 0;
			case  0: return 1;
			case  4: return 2;
			case  1: return 3;
			case  8: return 4;
			case 12: return 5;
			case 16: return 6;
			case 20: return 7;
			case 28: return 8;
			case 10: return 9; // screen 0 width 80
			default: return 10; // not a valid basic screen mode
		}
	}();

	// 2 vdps, 11 basic screenmodes, 12 registers
	static const int mustBeOne[][11][12] = {
		{ // TMS99x8 MSX1 VDP chip
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 0;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 1;
		{0, 0, 0x00, 0x7f, 0x03, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 2;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 3;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 4;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 5;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 6;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 7;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 8;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 0 width 80;
		{0, 0,    0,    0,    0,    0,    0,    0, 0,0, 0, 0} // undefined basic screen;
		},
		{ // V9938 and V9958 MSX2/2+ VDP chip
		// Note that there is a hardcoded check for reg5 bit 2 (A9) in spritemode 2
		// which deliberatly isn't set here in the table!
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 0;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 1;
		{0, 0, 0x00, 0x7f, 0x03, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 2;
		{0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 3;
		{0, 0, 0x00, 0x7f, 0x03, 0x03, 0x00, 0x00, 0,0,0,0}, // screen 4;
		{0, 0, 0x1f, 0x00, 0x00, 0x03, 0x00, 0x00, 0,0,0,0}, // screen 5;
		{0, 0, 0x1f, 0x00, 0x00, 0x03, 0x00, 0x00, 0,0,0,0}, // screen 6;
		{0, 0, 0x1f, 0x00, 0x00, 0x03, 0x00, 0x00, 0,0,0,0}, // screen 7;
		{0, 0, 0x1f, 0x00, 0x00, 0x03, 0x00, 0x00, 0,0,0,0}, // screen 8;
		{0, 0, 0x03, 0x07, 0x00, 0x00, 0x00, 0x00, 0,0,0,0}, // screen 0 width 80;
		{0, 0,    0,    0,    0,    0,    0,    0, 0,0,0,0} // undefined basic screen;
		}

	};
	static const int bitsused[][11][12] = {
		{// MSX1 = static const int VDP_TMS99X8 = 1
		{0,1, 0x0f, 0xff, 0x07, 0x7f, 0x07, 7,8,9, 0x00, 0x00}, // screen 0;
		{0,1, 0x0f, 0xff, 0x07, 0x7f, 0x07, 7,8,9, 0x00, 0x00}, // screen 1;
		{0,1, 0x0f, 0xff, 0x07, 0x7f, 0x07, 7,8,9, 0x00, 0x00}, // screen 2;
		{0,1, 0x0f, 0xff, 0x07, 0x7f, 0x07, 7,8,9, 0x00, 0x00}, // screen 3;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 4;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 5;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 6;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 7;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 8;
		{0,1, 0x00, 0x00, 0x00, 0x00, 0x00, 7,8,9, 0x00, 0x00}, // screen 0 width 80;
		{255,255,255,255,255,255,255,255,255,255,255,255} // undefined basic screen;
		},
		{ // MSX2 = static const int VDP_V9938 = 0;
		{0,1, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 7,8,9, 0x07, 0x03}, // screen 0;
		{0,1, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 7,8,9, 0x07, 0x03}, // screen 1;
		{0,1, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 7,8,9, 0x07, 0x03}, // screen 2;
		{0,1, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 7,8,9, 0x07, 0x03}, // screen 3;
		{0,1, 0x7f, 0xff, 0x3f, 0xfc, 0x3f, 7,8,9, 0x07, 0x03}, // screen 4;
		{0,1, 0x7f, 0xff, 0x3f, 0xfc, 0x3f, 7,8,9, 0x07, 0x03}, // screen 5;
		{0,1, 0x7f, 0xff, 0x3f, 0xfc, 0x3f, 7,8,9, 0x07, 0x03}, // screen 6;
		{0,1, 0x7f, 0xff, 0x3f, 0xfc, 0x3f, 7,8,9, 0x07, 0x03}, // screen 7;
		{0,1, 0x7f, 0xff, 0x3f, 0xfc, 0x3f, 7,8,9, 0x07, 0x03}, // screen 8;
		{0,1, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 7,8,9, 0x07, 0x03}, // screen 0 width 80;
		{255,255,255,255,255,255,255,255,255,255,255,255} // undefined basic screen;
		}
	};

	// update all the individual bits
	int upper_r = (vdpId == VDP_TMS99X8) ? 7 : (vdpId == VDP_V9938) ? 23 : 27;
	for (int r = 0; r <= upper_r; ++r) {
		if (r == 24) continue;
		for (int b = 7; b >= 0; --b) {
			QString name = QString("pushButton_%1_%2").arg(r).arg(b);
			auto* i = findChild<InteractiveButton*>(name);
			i->setChecked(regs[r] & (1 << b));
			if (r < 12) {
				i->mustBeSet(mustBeOne[(vdpId == VDP_TMS99X8) ? 0 : 1][basicScreen][r] & (1 << b));
				// if A8 of R5 is a 'must-be-one' then we indicate this for A9 also
				// This bit is cleared in the table since it isn't used in the Sprite
				// Attribute Table address calculation otherwise, but will only impact the
				// Sprite Color Table
				if (r == 5 && b == 2 && vdpId != VDP_TMS99X8 && mustBeOne[1][basicScreen][5]) {
					i->mustBeSet(true);
				}
			}
		}
	}

	// Start the interpretation
	label_dec_ie2->setText((regs[0] & 32)
		? "Interrupt from lightpen enabled"
		: "Interrupt from lightpen disabled");
	label_dec_ie1->setText((regs[0] & 16)
		? "Reg 19 scanline interrupt enabled"
		: "Reg 19 scanline interrupt disabled");

	if (vdpId != VDP_TMS99X8) {
		if (m == 20 || m == 28) {
			pushButton_2_6->setText("0");
			pushButton_2_5->setText("A16");
			pushButton_2_4->setText("A15");
			pushButton_2_3->setText("A14");
			pushButton_2_2->setText("A13");
			pushButton_2_1->setText("A12");
			pushButton_2_0->setText("A11");
		} else {
			pushButton_2_6->setText("A16");
			pushButton_2_5->setText("A15");
			pushButton_2_4->setText("A14");
			pushButton_2_3->setText("A13");
			pushButton_2_2->setText("A12");
			pushButton_2_1->setText("A11");
			pushButton_2_0->setText("A10");
		}
	}

	if (m == 28 && vdpId == VDP_V9958) {
		bool yjk = (regs[25] &  8);
		bool yae = (regs[25] & 16);
		int scr = yjk ? (yae ? 10 : 12) : 8;
		label_dec_m->setText(QString("M=%1%2%3 : SCREEN %4")
			.arg(m)
			.arg(yjk ? "+YJK" : "")
			.arg(yae ? "+YAE" : "")
			.arg(scr));
	} else {
		label_dec_m->setText(QString("M=%1 :  %2")
			.arg(m)
			.arg(screen[m]));
	}

	label_dec_bl->setText((regs[1] & 64)
		? "Display enabled"
		: "Display disabled");
	label_dec_ie0->setText((regs[1] & 32)
		? "V-Blank interrupt enabled"
		: "V-Blank interrupt disabled");

	label_dec_si->setText((regs[1] & 2)
		? "16x16 sprites"
		: "8x8 sprites");
	label_dec_mag->setText((regs[1] & 1)
		? "magnified sprites"
		: "regular sized");



	// Now calculate the addresses of all tables
	// TODO : clean up code and get rid of all the mustbeset bits for regs>5 since there are never must bits.


	// some variables used for readability of the code below
	int row = vdpId == VDP_TMS99X8 ? 0 : 1;
	QString regtexttext;
	int must, must2;

	// the pattern name table address
	must = mustBeOne[row][basicScreen][2] ;
	long nameTable = ((255^must) & bitsused[row][basicScreen][2] & regs[2]) << 10;
	if ((m == 20 || m == 28) && vdpId != VDP_TMS99X8)
		nameTable = ((nameTable & 0xffff) << 1) | ((nameTable & 0x10000) >> 16);
	regtexttext = hex5(nameTable);

	if ((must & regs[2]) != must) {
		label_dec_r2->setText("<font color=red>" + regtexttext + "</font>");
		label_dec_r2->setToolTip("Some of the obligatory 1 bits are reset!");
	} else {
		label_dec_r2->setText(regtexttext);
		label_dec_r2->setToolTip(nullptr);
	}

	// the color table address
	must  = mustBeOne[row][basicScreen][3];
	must2 = mustBeOne[row][basicScreen][10];
	regtexttext = hex5(
		(
			((255 ^ must ) & bitsused[row][basicScreen][ 3] & regs[ 3]) <<  6
		  ) | (
			((255 ^ must2) & bitsused[row][basicScreen][10] & regs[10]) << 14
		)
		);
	if (((must & regs[3]) != must) || ((must2 & regs[10]) != must2)) {
		label_dec_r3->setText("<font color=red>" + regtexttext + "</font>");
		label_dec_r3->setToolTip("Some of the obligatory 1 bits are reset!");
	} else {
		label_dec_r3->setText(regtexttext);
		label_dec_r3->setToolTip(nullptr);
	}

	// the pattern generator address
	must = mustBeOne[row][basicScreen][4] ;
	regtexttext = hex5(
		(
			(255 ^ must) & bitsused[row][basicScreen][4] & regs[4]) << 11
		);
	if ((must & regs[4]) != must) {
		label_dec_r4->setText("<font color=red>" + regtexttext + "</font>");
		label_dec_r4->setToolTip("Some of the obligatory 1 bits are reset!");
	} else {
		label_dec_r4->setText(regtexttext);
		label_dec_r4->setToolTip(nullptr);
	}

	// the sprite attribute tabel address
	must  = mustBeOne[row][basicScreen][ 5];
	must2 = mustBeOne[row][basicScreen][11];
	regtexttext = hex5(
		(
		(((255^must) & bitsused[row][basicScreen][ 5] & regs[ 5]) <<  7) |
		(((255^must2) & bitsused[row][basicScreen][11] & regs[11]) << 15))
		);
	if (((must & regs[5]) != must) || ((must2 & regs[11]) != must2)) {
		label_dec_r5->setText("<font color=red>" + regtexttext + "</font>");
		label_dec_r5->setToolTip("Some of the obligatory 1 bits are reset!");
	} else {
		label_dec_r5->setText(regtexttext);
		label_dec_r5->setToolTip(nullptr);
	}
	// special case for sprite mode 2
	if (must && !(4 & regs[ 5])) {  // only in mode2 there are some 'must'-bits :-)
		label_dec_r5->setText("<font color=red>" + regtexttext + "</font>");
		label_dec_r5->setToolTip("Bit A9 should be set, to obtain the Sprite Color Table address this bit is masked<br>With the current bit reset the Color Tabel will use the same address as the Sprite Attribute Table!");
	}


	// the sprite pattern generator address
	label_dec_r6->setText(hex5(
		((255^mustBeOne[row][basicScreen][6]) & bitsused[row][basicScreen][6] & regs[6]) << 11));

	// end of address calculations

	label_dec_tc->setText(dec2((regs[7] >> 4) & 15));
	label_dec_bd->setText(dec2((regs[7] >> 0) & 15));

	if (vdpId != VDP_TMS99X8) {
		label_dec_dg->setText((regs[0] & 64)
			? "Color bus set for input"
			: "Color bus set for output");
		label_dec_tp->setText((regs[8] & 32)
			? "Color 0 uses the color registers"
			: "Color 0 is transparent (=shows border)");
		label_dec_spd->setText((regs[8] & 2)
			? "Sprites disabled"
			: "Sprites enabled");

		label_dec_ln->setText((regs[9] & 128) ? "212" : "192");
		label_dec_il->setText((regs[9] &   8) ? "interlaced" : "non-interlaced");
		label_dec_eo->setText((regs[9] &   4) ? "alternate pages" : "same page");
		label_dec_nt->setText((regs[9] &   2) ? "PAL" : "NTSC");

		label_dec_t2 ->setText(dec2((regs[12] >> 4) & 15));
		label_dec_bc ->setText(dec2((regs[12] >> 0) & 15));
		label_dec_on ->setText(dec2((regs[13] >> 4) & 15));
		label_dec_off->setText(dec2((regs[13] >> 0) & 15));

		int x = ((regs[18] >> 0) & 15);
		int y = ((regs[18] >> 4) & 15);
		x = x > 7 ? 16 - x : -x;
		y = y > 7 ? 16 - y : -y;
		label_dec_r18->setText(QString("(%1,%2)").arg(x).arg(y));

		label_dec_r19->setText(dec3(regs[19]));
		label_dec_r23->setText(dec3(regs[23]));

		label_dec_r14->setText(hex5(((regs[14] & 7) << 14) | ((regs[81] & 63) << 8) | regs[80]));
		label_dec_r15->setText(dec2(regs[15] & 15));
		label_dec_r16->setText(dec2(regs[16] & 15));
		label_dec_r17->setText(dec3(regs[17] & 63).append((regs[17] & 128) ? "" : ", auto incr"));
	}

	//V9958 registers
	if (vdpId == VDP_V9958) {
		label_dec_r26->setText(QString("horizontal scroll %1")
			.arg((regs[26] & 63) * 8 - (7 & regs[27])));
		label_dec_sp2->setText((regs[25] & 1)
			? "Scroll uses 2 pages"
			: "Scroll same page");
		label_dec_msk->setText((regs[25] & 2)
			? "Hide 8 leftmost pixels"
			: "No masking");
		label_dec_wte->setText((regs[25] & 4)
			? "CPU Waitstate enabled"
			: "CPU Waitstate disabled");
		if (regs[25] & 8) {
			label_dec_yjk->setText("YJK System");
			label_dec_yae->setText((regs[25] & 16)
				? "Attribute enabled (Y=4bits)"
				: "regular YJK  (Y=5bits)");
		} else {
			label_dec_yjk->setText("Normal RGB");
			label_dec_yae->setText("Ignored (YJK disabled)");
		}
		label_dec_vds->setText((regs[25] & 32)
			? "Pin8 is /VDS"
			: "Pin8 is CPUCLK");
		label_dec_cmd->setText((regs[25] & 64)
			? "CMD engine in char modes"
			: "V9938 VDP CMD engine");
	}
}

void VDPRegViewer::doConnect(InteractiveButton* but, buttonHighlightDispatcher* dis)
{
	connect(but, &InteractiveButton::mouseOver,
	        dis, &buttonHighlightDispatcher::receiveState);
	connect(dis, &buttonHighlightDispatcher::stateDispatched,
	        but, &InteractiveButton::highlight);
}

void VDPRegViewer::monoGroup(InteractiveButton* but, InteractiveLabel* lab)
{
	connect(lab, &InteractiveLabel ::mouseOver, but, &InteractiveButton::highlight);
	connect(but, &InteractiveButton::mouseOver, lab, &InteractiveLabel ::highlight);
	connect(lab, &InteractiveLabel ::mouseOver, lab, &InteractiveLabel ::highlight);
	connect(but, &InteractiveButton::mouseOver, but, &InteractiveButton::highlight);
}

void  VDPRegViewer::reGroup(InteractiveButton* item, buttonHighlightDispatcher* dispat)
{
	//button must re-highlight itself
	connect(item, &InteractiveButton::mouseOver, item, &InteractiveButton::highlight);
	// and then talk to dispatcher
	doConnect(item, dispat);
}

buttonHighlightDispatcher* VDPRegViewer::makeGroup(
		const QList<InteractiveButton*>& list, InteractiveLabel* explained)
{
	// First "steal" the Tooltip of the explained widget.
	//for (auto* item : list) {
	//	item->setToolTip(explained->toolTip());
	//}

	// Create a dispatcher and connect all to them
	auto* dispat = new buttonHighlightDispatcher();
	connect(explained, &InteractiveLabel::mouseOver,
	        dispat, &buttonHighlightDispatcher::receiveState);
	connect(dispat, &buttonHighlightDispatcher::stateDispatched,
	        explained, &InteractiveLabel::highlight);
	for (auto* item : list) {
		doConnect(item, dispat);
	}
	return dispat;
}

void VDPRegViewer::connectHighLights()
{
	// Before connecting the highlights we connect the special
	// 'Toggled Bit' event
	// Warning: This function is not available with MSVC 6!! Not that it
	// matters to me on my Linux environment :-)
	QList<InteractiveButton*> list = findChildren<InteractiveButton*>();
	for (auto* item : list) {
		connect(item, &InteractiveButton::newBitValue,
		        this, &VDPRegViewer::registerBitChanged);
        }

	// register 0 (+M1,M2)
	monoGroup(pushButton_0_5, label_dec_ie2);
	monoGroup(pushButton_0_4, label_dec_ie1);

	list.clear();
	list << pushButton_0_3 << pushButton_0_2 << pushButton_0_1;
	list << pushButton_1_4 << pushButton_1_3;
	modeBitsDispat = makeGroup(list, label_dec_m);

	// register 1
	monoGroup(pushButton_1_6, label_dec_bl);
	monoGroup(pushButton_1_5, label_dec_ie0);
	monoGroup(pushButton_1_1, label_dec_si);
	monoGroup(pushButton_1_0, label_dec_mag);

	// register 8
	//monoGroup(pushButton_8_7, label_dec_ms);
	//monoGroup(pushButton_8_6, label_dec_lp);
	monoGroup(pushButton_8_5, label_dec_tp);
	//monoGroup(pushButton_8_4, label_dec_cb);
	//monoGroup(pushButton_8_3, label_dec_vr);
	monoGroup(pushButton_8_1, label_dec_spd);
	//monoGroup(pushButton_8_0, label_dec_bw);

	// register 9
	monoGroup(pushButton_9_7, label_dec_ln);
	list.clear();
	list << pushButton_9_5 << pushButton_9_4;
	makeGroup(list, label_dec_m); //TODO fix label
	monoGroup(pushButton_9_3, label_dec_il);
	monoGroup(pushButton_9_2, label_dec_eo);
	monoGroup(pushButton_9_1, label_dec_nt);
	//monoGroup(pushButton_9_0, label_dec_dc);

	// register 2
	list.clear();
	list << pushButton_2_7 << pushButton_2_6 << pushButton_2_5;
	list << pushButton_2_4 << pushButton_2_3 << pushButton_2_2;
	list << pushButton_2_1 << pushButton_2_0;
	makeGroup(list, label_dec_r2);

	// register 3 + 10
	list.clear();
	list << pushButton_3_7 << pushButton_3_6 << pushButton_3_5;
	list << pushButton_3_4 << pushButton_3_3 << pushButton_3_2;
	list << pushButton_3_1 << pushButton_3_0;
	list << pushButton_10_7 << pushButton_10_6 << pushButton_10_5;
	list << pushButton_10_4 << pushButton_10_3 << pushButton_10_2;
	list << pushButton_10_1 << pushButton_10_0;
	makeGroup(list, label_dec_r3);

	// register 4
	list.clear();
	list << pushButton_4_7 << pushButton_4_6 << pushButton_4_5;
	list << pushButton_4_4 << pushButton_4_3 << pushButton_4_2;
	list << pushButton_4_1 << pushButton_4_0;
	makeGroup(list, label_dec_r4);

	// register 5 + 11
	list.clear();
	list << pushButton_5_7 << pushButton_5_6 << pushButton_5_5;
	list << pushButton_5_4 << pushButton_5_3 << pushButton_5_2;
	list << pushButton_5_1 << pushButton_5_0;
	list << pushButton_11_7 << pushButton_11_6 << pushButton_11_5;
	list << pushButton_11_4 << pushButton_11_3 << pushButton_11_2;
	list << pushButton_11_1 << pushButton_11_0;
	makeGroup(list, label_dec_r5);

	// register 6
	list.clear();
	list << pushButton_6_7 << pushButton_6_6 << pushButton_6_5;
	list << pushButton_6_4 << pushButton_6_3 << pushButton_6_2;
	list << pushButton_6_1 << pushButton_6_0;
	makeGroup(list, label_dec_r6);

	// register 7
	list.clear();
	list << pushButton_7_7 << pushButton_7_6;
	list << pushButton_7_5 << pushButton_7_4;
	makeGroup(list, label_dec_tc);

	list.clear();
	list << pushButton_7_3 << pushButton_7_2;
	list << pushButton_7_1 << pushButton_7_0;
	makeGroup(list, label_dec_bd);

	// register 12
	list.clear();
	list << pushButton_12_7 << pushButton_12_6;
	list << pushButton_12_5 << pushButton_12_4;
	makeGroup(list, label_dec_t2);

	list.clear();
	list << pushButton_12_3 << pushButton_12_2;
	list << pushButton_12_1 << pushButton_12_0;
	makeGroup(list, label_dec_bc);

	// register 13
	list.clear();
	list << pushButton_13_7 << pushButton_13_6;
	list << pushButton_13_5 << pushButton_13_4;
	makeGroup(list, label_dec_on);

	list.clear();
	list << pushButton_13_3 << pushButton_13_2;
	list << pushButton_13_1 << pushButton_13_0;
	makeGroup(list, label_dec_off);

	// register 14
	list.clear();
	list << pushButton_14_7 << pushButton_14_6 << pushButton_14_5;
	list << pushButton_14_4 << pushButton_14_3 << pushButton_14_2;
	list << pushButton_14_1 << pushButton_14_0;
	makeGroup(list, label_dec_r14);

	// register 15
	list.clear();
	list << pushButton_15_7 << pushButton_15_6 << pushButton_15_5;
	list << pushButton_15_4 << pushButton_15_3 << pushButton_15_2;
	list << pushButton_15_1 << pushButton_15_0;
	makeGroup(list, label_dec_r15);

	// register 16
	list.clear();
	list << pushButton_16_7 << pushButton_16_6 << pushButton_16_5;
	list << pushButton_16_4 << pushButton_16_3 << pushButton_16_2;
	list << pushButton_16_1 << pushButton_16_0;
	makeGroup(list, label_dec_r16);

	// register 17
	list.clear();
	list << pushButton_17_7 << pushButton_17_6 << pushButton_17_5;
	list << pushButton_17_4 << pushButton_17_3 << pushButton_17_2;
	list << pushButton_17_1 << pushButton_17_0;
	makeGroup(list, label_dec_r17);

	// register 18
	list.clear();
	list << pushButton_18_7 << pushButton_18_6 << pushButton_18_5;
	list << pushButton_18_4 << pushButton_18_3 << pushButton_18_2;
	list << pushButton_18_1 << pushButton_18_0;
	makeGroup(list, label_dec_r18);

	// register 19
	list.clear();
	list << pushButton_19_7 << pushButton_19_6 << pushButton_19_5;
	list << pushButton_19_4 << pushButton_19_3 << pushButton_19_2;
	list << pushButton_19_1 << pushButton_19_0;
	makeGroup(list, label_dec_r19);

	// register 23
	list.clear();
	list << pushButton_23_7 << pushButton_23_6 << pushButton_23_5;
	list << pushButton_23_4 << pushButton_23_3 << pushButton_23_2;
	list << pushButton_23_1 << pushButton_23_0;
	makeGroup(list, label_dec_r23);

	// register 25
	monoGroup(pushButton_25_0, label_dec_sp2);
	monoGroup(pushButton_25_1, label_dec_msk);
	monoGroup(pushButton_25_2, label_dec_wte);
	monoGroup(pushButton_25_3, label_dec_yjk);
	monoGroup(pushButton_25_4, label_dec_yae);
	monoGroup(pushButton_25_5, label_dec_vds);
	monoGroup(pushButton_25_6, label_dec_cmd);

	// register 26
	list.clear();
	list << pushButton_26_7 << pushButton_26_6 << pushButton_26_5;
	list << pushButton_26_4 << pushButton_26_3 << pushButton_26_2;
	list << pushButton_26_1 << pushButton_26_0;
	list << pushButton_27_7 << pushButton_27_6 << pushButton_27_5;
	list << pushButton_27_4 << pushButton_27_3 << pushButton_27_2;
	list << pushButton_27_1 << pushButton_27_0;
	makeGroup(list, label_dec_r26);
}

void VDPRegViewer::refresh()
{
	//new SimpleHexRequest("{VDP regs}", 0, 64, regs, *this);
	//new SimpleHexRequest("{VDP status regs}", 0, 16, regs, *this);
	// now combined in one request:
	new SimpleHexRequest(
		"debug_bin2hex "
		"[ debug read_block {VDP regs} 0 64 ]"
		"[ debug read_block {VDP status regs} 0 16 ]"
		"[ debug read_block {VRAM pointer} 0 2 ]",
		64 + 16 + 2, regs, *this);
}

void VDPRegViewer::DataHexRequestReceived()
{
	decodeStatusVDPRegs();
	decodeVDPRegs();
}

void VDPRegViewer::registerBitChanged(int reg, int bit, bool state)
{
	// maybe this call is the result of our own SetChecked (VDPDataStorte
	// update event)
	if ((state ? 1 : 0) == (regs[reg] & (1 << bit))) {
		return;
	}

	// state does not correspond to the current register setting so we send
	// an update command
	if (state) {
		regs[reg] |=  (1 << bit);
	} else {
		regs[reg] &= ~(1 << bit);
	}
	CommClient::instance().sendCommand(
		new SimpleCommand(
			QString("debug write {VDP regs} %1 %2").arg(reg).arg(regs[reg])));

	// Update display without waiting for the VDPDataStore update
	decodeVDPRegs();
	// and then we could request an update nevertheless since some other
	// objects might want to see this change through the VDPDataStore also
	// :-)
	// VDPDataStore::instance().refresh();
}

void VDPRegViewer::on_VDPcomboBox_currentIndexChanged(int index)
{
	switch (index) {
	case 0:
		vdpId = VDP_V9958;
		break;
	case 1:
		vdpId = VDP_V9938;
		break;
	case 2:
		vdpId = VDP_TMS99X8;
		break;
	}
	decodeStatusVDPRegs();
	decodeVDPRegs();
}
