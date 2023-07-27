#include "MainMemoryViewer.h"
#include "HexViewer.h"
#include "CPURegs.h"
#include "CPURegsViewer.h"
#include "SymbolTable.h"
#include "Convert.h"
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <iostream>

static const int linkRegisters[] = {
	CpuRegs::REG_BC, CpuRegs::REG_DE, CpuRegs::REG_HL,
	CpuRegs::REG_IX, CpuRegs::REG_IY,
	CpuRegs::REG_BC2, CpuRegs::REG_DE2, CpuRegs::REG_HL2,
	CpuRegs::REG_PC, CpuRegs::REG_SP
};

MainMemoryViewer::MainMemoryViewer(QWidget* parent)
	: QWidget(parent)
{
	// create selection list, address edit line and viewer
	addressSourceList = new QComboBox();
	addressSourceList->setEditable(false);
	addressSourceList->addItem("Address:");
	for (int lr : linkRegisters) {
		QString txt = QString("Linked to ");
		txt.append(CpuRegs::regNames[lr]);
		addressSourceList->addItem(txt);
	}

	addressValue = new QLineEdit();
	addressValue->setText(hexValue(0, 4));
	//addressValue->setEditable(false);

	hexView = new HexViewer();
	hexView->setUseMarker(true);
	hexView->setIsEditable(true);
	hexView->setIsInteractive(true);
	hexView->setDisplayMode(HexViewer::FILL_WIDTH_POWEROF2);
	auto* hbox = new QHBoxLayout();
	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->addWidget(addressSourceList);
	hbox->addWidget(addressValue);

	auto* vbox = new QVBoxLayout();
	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->addLayout(hbox);
	vbox->addWidget(hexView);
	setLayout(vbox);

	isLinked = false;
	linkedId = 0;
	regsViewer = nullptr;
	symTable = nullptr;

	connect(hexView, &HexViewer::locationChanged,
	        this, &MainMemoryViewer::hexViewChanged);
	connect(addressValue, &QLineEdit::returnPressed,
	        this, &MainMemoryViewer::addressValueChanged);
	connect(addressSourceList, qOverload<int>(&QComboBox::currentIndexChanged),
	        this, &MainMemoryViewer::addressSourceListChanged);
}

void MainMemoryViewer::settingsChanged()
{
	hexView->settingsChanged();
}

void MainMemoryViewer::setLocation(int addr)
{
	addressValue->setText(hexValue(addr, 4));
	hexView->setLocation(addr);
}

void MainMemoryViewer::setDebuggable(const QString& name, int size)
{
	hexView->setDebuggable(name, size);
}

void MainMemoryViewer::setRegsView(CPURegsViewer* viewer)
{
	regsViewer = viewer;
}

void MainMemoryViewer::setSymbolTable(SymbolTable* symtable)
{
	symTable = symtable;
}

void MainMemoryViewer::refresh()
{
	hexView->refresh();
}

void MainMemoryViewer::hexViewChanged(int addr)
{
	addressValue->setText(hexValue(addr, 4));
}

void MainMemoryViewer::addressValueChanged()
{
	auto addr = stringToValue<uint16_t>(addressValue->text());
	if (!addr && symTable) {
		// try finding a label
		Symbol *s = symTable->getAddressSymbol(addressValue->text());
		if (!s) s = symTable->getAddressSymbol(addressValue->text(), Qt::CaseInsensitive);
		if (s) addr = s->value();
	}

	if (addr) hexView->setLocation(*addr);
}

void MainMemoryViewer::registerChanged(int id, int value)
{
	if (!isLinked || (id != linkedId)) {
		return;
	}

	addressValue->setText(hexValue(value, 4));
	hexView->setLocation(value);
	//hexView->refresh();
}

void MainMemoryViewer::addressSourceListChanged(int index)
{
	if (index == 0) {
		isLinked = false;
		addressValue->setReadOnly(false);
		hexView->setIsInteractive(true);
	} else {
		isLinked = true;
		linkedId = linkRegisters[index - 1];
		addressValue->setReadOnly(true);
		hexView->setIsInteractive(false);
		if (regsViewer) {
			setLocation(regsViewer->readRegister(linkedId));
		}
	}
}
