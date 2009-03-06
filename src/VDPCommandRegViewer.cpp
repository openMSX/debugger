#include "VDPCommandRegViewer.h"
#include "CommClient.h"

static const int VIEW_HEX = 1;
static const int VIEW_SEPERATE = 2;

static QString hex8bit(int val)
{
	return QString("0x%1").arg(val, 2, 16, QChar('0')).toUpper();
}

VDPCommandRegViewer::VDPCommandRegViewer(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	regs = new unsigned char[64 + 16];
	memset(regs, 0, 64 + 16);
	statusregs = regs + 64;

	// create the needed groups
	grp_sx = new view88to16(lineEdit_r32, lineEdit_r33, lineEdit_sx);
	connect(lineEdit_r32, SIGNAL(editingFinished()), grp_sx, SLOT(finishRL()));
	connect(lineEdit_r33, SIGNAL(editingFinished()), grp_sx, SLOT(finishRH()));
	connect(lineEdit_sx,  SIGNAL(editingFinished()), grp_sx, SLOT(finishRW()));
	grp_sy = new view88to16(lineEdit_r34, lineEdit_r35, lineEdit_sy);
	connect(lineEdit_r34, SIGNAL(editingFinished()), grp_sy, SLOT(finishRL()));
	connect(lineEdit_r35, SIGNAL(editingFinished()), grp_sy, SLOT(finishRH()));
	connect(lineEdit_sy,  SIGNAL(editingFinished()), grp_sy, SLOT(finishRW()));
	grp_dx = new view88to16(lineEdit_r36, lineEdit_r37, lineEdit_dx);
	connect(lineEdit_r36, SIGNAL(editingFinished()), grp_dx, SLOT(finishRL()));
	connect(lineEdit_r37, SIGNAL(editingFinished()), grp_dx, SLOT(finishRH()));
	connect(lineEdit_dx,  SIGNAL(editingFinished()), grp_dx, SLOT(finishRW()));
	grp_dy = new view88to16(lineEdit_r38, lineEdit_r39, lineEdit_dy);
	connect(lineEdit_r38, SIGNAL(editingFinished()), grp_dy, SLOT(finishRL()));
	connect(lineEdit_r39, SIGNAL(editingFinished()), grp_dy, SLOT(finishRH()));
	connect(lineEdit_dy,  SIGNAL(editingFinished()), grp_dy, SLOT(finishRW()));
	grp_nx = new view88to16(lineEdit_r40, lineEdit_r41, lineEdit_nx);
	connect(lineEdit_r40, SIGNAL(editingFinished()), grp_nx, SLOT(finishRL()));
	connect(lineEdit_r41, SIGNAL(editingFinished()), grp_nx, SLOT(finishRH()));
	connect(lineEdit_nx,  SIGNAL(editingFinished()), grp_nx, SLOT(finishRW()));
	grp_ny = new view88to16(lineEdit_r42, lineEdit_r43, lineEdit_ny);
	connect(lineEdit_r42, SIGNAL(editingFinished()), grp_ny, SLOT(finishRL()));
	connect(lineEdit_r43, SIGNAL(editingFinished()), grp_ny, SLOT(finishRH()));
	connect(lineEdit_ny,  SIGNAL(editingFinished()), grp_ny, SLOT(finishRW()));

	grp_sx->setRW(QString("0"));
	grp_sy->setRW(QString("0"));
	grp_dx->setRW(QString("0"));
	grp_dy->setRW(QString("0"));
	grp_nx->setRW(QString("0"));
	grp_ny->setRW(QString("0"));

	grp_l_sx = new view88to16(label_r_32, label_r_33, label_r_sx);
	grp_l_sy = new view88to16(label_r_34, label_r_35, label_r_sy);
	grp_l_dx = new view88to16(label_r_36, label_r_37, label_r_dx);
	grp_l_dy = new view88to16(label_r_38, label_r_39, label_r_dy);
	grp_l_nx = new view88to16(label_r_40, label_r_41, label_r_nx);
	grp_l_ny = new view88to16(label_r_42, label_r_43, label_r_ny);

	//Connect the checkboxes
	QList<QCheckBox*> list = this->findChildren<QCheckBox*>();
	QCheckBox* item;
	foreach(item, list) {
		connect(item, SIGNAL(stateChanged(int)),
		        this, SLOT(R45BitChanged(int)));
	}

	lineEdit_r44->setText("0x00");
	lineEdit_r45->setText("0x00");
	lineEdit_r46->setText("0x00");
	R46 = 0;
	comboBox_cmd->setCurrentIndex(0);
	//decodeR46(R46);
	//on_comboBox_cmd_currentIndexChanged();

	//get initiale data
	refresh();
}

VDPCommandRegViewer::~VDPCommandRegViewer()
{
	delete[] regs;
}

void VDPCommandRegViewer::on_lineEdit_r45_editingFinished()
{
	//for now simply recheck all the checkBoxes and recreate R45
	int val = lineEdit_r45->text().toInt(NULL, 0);
	int r45 = 0;
	QList<QCheckBox*> list = findChildren<QCheckBox*>();
	QCheckBox* item;
	foreach(item, list) {
	        int order = QString(item->objectName().right(1)).toInt();
		if (val & (1 << order)) {
			r45 = r45 | (1 << order);
			item->setChecked(true);
		} else {
			item->setChecked(false);
		}
	}
	lineEdit_r45->setText(QString("0x%1").arg(r45, 2, 16, QChar('0')).toUpper());
	label_arg->setText(QString("%1").arg(r45, 8, 2, QChar('0')));
}

void VDPCommandRegViewer::on_comboBox_cmd_currentIndexChanged(int index)
{
	if ((index < 7 || index > 9) && index != 11 && index != 5) {
		comboBox_operator->setEnabled(false);
		comboBox_operator->setCurrentIndex(5);
	} else {
		if (!comboBox_operator->isEnabled()) {
			comboBox_operator->setCurrentIndex(R46 & 15);
		}
		comboBox_operator->setEnabled(true);
	}
	R46 = (15 & R46) + 16 * index;
	decodeR46(R46);
	lineEdit_r46->setText(QString("0x%1").arg(R46, 2, 16, QChar('0')).toUpper());
}

void VDPCommandRegViewer::on_comboBox_operator_currentIndexChanged(int index)
{
	//if the cmd disables the combox then no influence on R45
	if (!comboBox_operator->isEnabled()) return;

	R46 = index + (R46 & 0xF0);
	decodeR46(R46);
	lineEdit_r46->setText(QString("0x%1").arg(R46, 2, 16, QChar('0')).toUpper());
}

void VDPCommandRegViewer::decodeR46(int val)
{
	static const QString words[16] = {
		"-", "VDP", "VRAM", "CPU", "Byte", "Dot", "Stop", "Invalid",
		"Point", "Pset", "Search", "Line", "Logical move", "High-speed move"
	};
	static const int dest[16] = {
		0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2 //refers to words
	};
	static const int srce[16] = {
		0, 0, 0, 0, 2, 1, 1, 1, 1, 2, 2, 3, 1, 2, 2, 3 //refers to words
	};
	static const int data[16] = {
		0, 0, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4 //refers to words
	};
	static const int cnam[16] = {
		6, 7, 7, 7, 8, 9,10,11,12,12,12,12,13,13,13,13 //refers to words
	};
	static const QString oper[16] = {
		"IMP",  "AND",  "OR",  "EOR",  "NOT",  "---", "---", "---",
		"TIMP", "TAND", "TOR", "TEOR", "TNOT", "---", "---", "---",
	};
	static const int argbold[16] = {
		0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x26, 0x2D,
		0x2C, 0x2C, 0x1C, 0x2C, 0x2C, 0x3C, 0x2C, 0x2C,
	};
	static const int regbold[16] = {
		0x00, 0x00, 0x00, 0x00, 0x83, 0xCC, 0xC3, 0xFC,
		0xFC, 0xBF, 0xB3, 0xFC, 0xFC, 0xBF, 0xAE, 0xFC,
	};

	int operat = (val >> 0) & 15;
	int cmd    = (val >> 4) & 15;

	QString explication = QString(
		"Command: <b>%1</b> "
		"Destination: <b>%2</b> "
		"Source: <b>%3</b><br>"
		"Data: <b>%4</b> "
		"Operation: <b>%5</b>").
			arg(words[cnam[cmd]]).
			arg(words[dest[cmd]]).
			arg(words[srce[cmd]]).
			arg(words[data[cmd]]).
			arg(oper[operat]);
	label_expl1->setText(explication);

	//a Normal and a Bold font
	QFont fontN;
	QFont fontB;
	fontB.setBold(true);
	//set the checkbox font in bold if command uses the corresponding bit
	for (int b = 0; b < 7; ++b) {
		QCheckBox* item = findChild<QCheckBox*>(QString("checkBoxBit_%1").arg(b));
		item->setFont((argbold[cmd] & (1 << b)) ? fontB : fontN);
	}
	labelSX   ->setFont((regbold[cmd] & 0x01) ? fontB : fontN);
	labelSY   ->setFont((regbold[cmd] & 0x02) ? fontB : fontN);
	labelDX   ->setFont((regbold[cmd] & 0x04) ? fontB : fontN);
	labelDY   ->setFont((regbold[cmd] & 0x08) ? fontB : fontN);
	labelNX   ->setFont((regbold[cmd] & 0x10) ? fontB : fontN);
	labelNY   ->setFont((regbold[cmd] & 0x20) ? fontB : fontN);
	labelCOLOR->setFont((regbold[cmd] & 0x40) ? fontB : fontN);
	labelARG  ->setFont((regbold[cmd] & 0x80) ? fontB : fontN);

	//TODO: create basic equivalent
	//TODO: warn about wrong stuff (majorant < minorant when line drawing for instance)
}

void VDPCommandRegViewer::on_syncPushButton_clicked()
{
	syncRegToCmd();
}

void VDPCommandRegViewer::on_launchPushButton_clicked()
{
	unsigned char newregs[64];
	memset(newregs, 0, 64);
	newregs[32] = lineEdit_r32->text().toInt(NULL, 0);
	newregs[33] = lineEdit_r33->text().toInt(NULL, 0);
	newregs[34] = lineEdit_r34->text().toInt(NULL, 0);
	newregs[35] = lineEdit_r35->text().toInt(NULL, 0);
	newregs[36] = lineEdit_r36->text().toInt(NULL, 0);
	newregs[37] = lineEdit_r37->text().toInt(NULL, 0);
	newregs[38] = lineEdit_r38->text().toInt(NULL, 0);
	newregs[39] = lineEdit_r39->text().toInt(NULL, 0);
	newregs[40] = lineEdit_r40->text().toInt(NULL, 0);
	newregs[41] = lineEdit_r41->text().toInt(NULL, 0);
	newregs[42] = lineEdit_r42->text().toInt(NULL, 0);
	newregs[43] = lineEdit_r43->text().toInt(NULL, 0);
	newregs[44] = lineEdit_r44->text().toInt(NULL, 0);
	newregs[45] = lineEdit_r45->text().toInt(NULL, 0);
	newregs[46] = lineEdit_r46->text().toInt(NULL, 0);

	WriteDebugBlockCommand* req = new WriteDebugBlockCommand(
		"{VDP regs}", 32, 15, newregs);
	CommClient::instance().sendCommand(req);
}

void VDPCommandRegViewer::on_lineEdit_r44_editingFinished()
{
	int val = lineEdit_r44->text().toInt(NULL, 0);
	label_color->setText(QString("%1 %2").
			arg((val >> 4) & 15, 4, 2, QChar('0')).
			arg((val >> 0) & 15, 4, 2, QChar('0')));
}

void VDPCommandRegViewer::R45BitChanged(int state)
{
	//for now simply recheck all the checkBoxes and recreate R45
	int r45 = 0;
	QList<QCheckBox*> list = findChildren<QCheckBox*>();
	QCheckBox* item;
	foreach(item, list) {
		if (item->isChecked()) {
			int order = QString(item->objectName().right(1)).toInt();
			r45 = r45 | (1 << order);
		}
	}
	lineEdit_r45->setText(QString("0x%1").arg(r45, 2, 16, QChar('0')).toUpper());
	label_arg->setText(QString("%1").arg(r45, 8, 2, QChar('0')));
}

void VDPCommandRegViewer::on_lineEdit_r46_editingFinished()
{
	int val = lineEdit_r46->text().toInt(NULL, 0) & 0xFF;
	lineEdit_r46->setText(QString("0x%1").arg(val, 2, 16, QChar('0')).toUpper());
	R46 = val;
	decodeR46(R46);
	comboBox_operator->setCurrentIndex(R46 & 15);
	comboBox_cmd->setCurrentIndex((R46 >> 4) & 15); // this might hide the operator again :-)
}

void VDPCommandRegViewer::refresh()
{
	//new SimpleHexRequest("{VDP regs}",0,64,regs, *this);
	//new SimpleHexRequest("{VDP status regs}",0,16,regs, *this);
	// now combined in one request:
	new SimpleHexRequest(
		"debug_bin2hex "
		"[ debug read_block {VDP regs} 0 64 ]"
		"[ debug read_block {VDP status regs} 0 16 ]",
		64 + 16, regs, *this);
}


void VDPCommandRegViewer::syncRegToCmd()
{
	grp_sx->setRL(hex8bit(regs[32]));
	grp_sx->setRH(hex8bit(regs[33]));
	grp_sy->setRL(hex8bit(regs[34]));
	grp_sy->setRH(hex8bit(regs[35]));
	grp_dx->setRL(hex8bit(regs[36]));
	grp_dx->setRH(hex8bit(regs[37]));
	grp_dy->setRL(hex8bit(regs[38]));
	grp_dy->setRH(hex8bit(regs[39]));
	grp_nx->setRL(hex8bit(regs[40]));
	grp_nx->setRH(hex8bit(regs[41]));
	grp_ny->setRL(hex8bit(regs[42]));
	grp_ny->setRH(hex8bit(regs[43]));

	lineEdit_r44->setText(hex8bit(regs[44]));
	on_lineEdit_r44_editingFinished();
	lineEdit_r45->setText(hex8bit(regs[45]));
	on_lineEdit_r45_editingFinished();
	lineEdit_r46->setText(hex8bit(regs[46]));
	on_lineEdit_r46_editingFinished();
}

void VDPCommandRegViewer::DataHexRequestReceived()
{
	grp_l_sx->setRL(hex8bit(regs[32]));
	grp_l_sx->setRH(hex8bit(regs[33]));
	grp_l_sy->setRL(hex8bit(regs[34]));
	grp_l_sy->setRH(hex8bit(regs[35]));
	grp_l_dx->setRL(hex8bit(regs[36]));
	grp_l_dx->setRH(hex8bit(regs[37]));
	grp_l_dy->setRL(hex8bit(regs[38]));
	grp_l_dy->setRH(hex8bit(regs[39]));
	grp_l_nx->setRL(hex8bit(regs[40]));
	grp_l_nx->setRH(hex8bit(regs[41]));
	grp_l_ny->setRL(hex8bit(regs[42]));
	grp_l_ny->setRH(hex8bit(regs[43]));
	label_r_44->setText(hex8bit(regs[44]));
	label_r_45->setText(hex8bit(regs[45]));
	label_r_46->setText(hex8bit(regs[46]));
	if (autoSyncRadioButton->isChecked()) syncRegToCmd();
}

