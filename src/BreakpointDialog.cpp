#include "BreakpointDialog.h"
#include "DebuggerData.h"
#include "Convert.h"

BreakpointDialog::BreakpointDialog(const MemoryLayout& ml, QWidget* parent)
	: QDialog(parent), memLayout(ml)
{
	setupUi(this);

	connect(cmbxSlot,    SIGNAL(activated(int)), this, SLOT(slotChanged(int)));
	connect(cmbxSubslot, SIGNAL(activated(int)), this, SLOT(subslotChanged(int)));

	slotChanged(0);
}

int BreakpointDialog::address()
{
	return stringToValue(edtAddress->text());
}

int BreakpointDialog::slot()
{
	return cmbxSlot->currentIndex() - 1;
}

int BreakpointDialog::subslot()
{
	return cmbxSubslot->currentIndex() - 1;
}

int BreakpointDialog::segment()
{
	return cmbxSegment->currentIndex() - 1;
}

void BreakpointDialog::slotChanged(int i)
{
	if (i) {
		if (memLayout.isSubslotted[i - 1]) {
			cmbxSubslot->setEnabled(true);
		} else {
			cmbxSubslot->setEnabled(false);
			cmbxSubslot->setCurrentIndex(0);
		}
	} else {
		cmbxSubslot->setEnabled(false);
		cmbxSubslot->setCurrentIndex(0);
	}
	subslotChanged(cmbxSubslot->currentIndex());
}

void BreakpointDialog::subslotChanged(int i)
{
	cmbxSegment->clear();
	cmbxSegment->addItem("any");
	
	int ps = cmbxSlot->currentIndex() - 1;
	int ss = i - 1;
	
	if (ps >=0 && !memLayout.isSubslotted[ps]) ss = 0;
	
	if (ps < 0 || ss < 0 || memLayout.mapperSize[ps][ss] == 0) {
		cmbxSegment->setEnabled(false);
		cmbxSegment->setCurrentIndex(0);
		return;
	}
	
	for (int s = 0; s < memLayout.mapperSize[ps][ss]; ++s) {
		cmbxSegment->addItem(QString("%1").arg(s));
	}
	cmbxSegment->setEnabled(true);
}
