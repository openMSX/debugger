#include "BreakpointDialog.h"
#include "DebuggerData.h"
#include "DebugSession.h"
#include "Convert.h"
#include <QCompleter>
#include <QStandardItemModel>


BreakpointDialog::BreakpointDialog(const MemoryLayout& ml, DebugSession *session, QWidget* parent)
	: QDialog(parent), memLayout(ml), currentSymbol(0)
{
	setupUi(this);

	debugSession = session;
	if( session ) {
		// create address completer
		QCompleter *completer = new QCompleter(session->symbolTable().labelList(), this);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		edtAddress->setCompleter(completer);
		connect(completer,  SIGNAL(activated(const QString&)), this, SLOT(addressChanged(const QString&)));
	}

	connect(edtAddress,  SIGNAL(textEdited(const QString&)), this, SLOT(addressChanged(const QString&)));
	connect(cmbxSlot,    SIGNAL(activated(int)), this, SLOT(slotChanged(int)));
	connect(cmbxSubslot, SIGNAL(activated(int)), this, SLOT(subslotChanged(int)));

	slotChanged(0);
	cmbxSlot->setEnabled(false);
	idxSlot = idxSubSlot = 0;
}

int BreakpointDialog::address()
{
	if(currentSymbol)
		return currentSymbol->value();
	else
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

void BreakpointDialog::addressChanged(const QString& text)
{
	int addr = stringToValue(text);
	if (addr == -1 && debugSession) {
		// try finding a label
		currentSymbol = debugSession->symbolTable().getAddressSymbol(text);
		if (!currentSymbol) currentSymbol = debugSession->symbolTable().getAddressSymbol(text, Qt::CaseInsensitive);
		if (currentSymbol) addr = currentSymbol->value();
	}

	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(cmbxSlot->model());
	if (addr == -1) {
		QPalette pal;
		pal.setColor(QPalette::Text, Qt::red);
		edtAddress->setPalette(pal);
		cmbxSlot->setCurrentIndex(0);
		cmbxSlot->setEnabled(false);
	} else {
		QPalette pal;
		pal.setColor(QPalette::Text, Qt::black);
		edtAddress->setPalette(pal);		
		cmbxSlot->setEnabled(true);

		if(currentSymbol) {
			// enable allowed slots
			int s = currentSymbol->validSlots();
			int num = 0, last = 0;
			for(int i = 4; i >= 0; i--) {
				QModelIndex index = model->index(i, 0, cmbxSlot->rootModelIndex());
				if(i) {
					bool ena = (s & (15<<(4*(i-1)))) != 0;
					model->itemFromIndex(index)->setEnabled(ena);
					if(ena) {
						num++;
						last = i;
					}	
				} else {
					model->itemFromIndex(index)->setEnabled( num == 4 );
					if(num == 4) last = 0;
				}
			}
			cmbxSlot->setCurrentIndex(last);
		} else {
			// enable everything
			for(int i = 0; i < 5; i++) {
				QModelIndex index = model->index(i, 0, cmbxSlot->rootModelIndex());
				model->itemFromIndex(index)->setEnabled(true);
			}
			cmbxSlot->setCurrentIndex(idxSlot);
		}
	}
	slotChanged(cmbxSlot->currentIndex());
}

void BreakpointDialog::slotChanged(int s)
{
	if(!currentSymbol) idxSlot = s;
	if (s && memLayout.isSubslotted[s - 1]) {
		if(currentSymbol) {
			// enable subslots
			int v = (currentSymbol->validSlots() >> (4*(s-1))) & 15;
			int num = 0, last = 0;
				QStandardItemModel* model = qobject_cast<QStandardItemModel*>(cmbxSubslot->model());
			for(int i = 4; i >= 0; i--) {
				QModelIndex index = model->index(i, 0, cmbxSubslot->rootModelIndex());
				if(i) {
					bool ena = (v & (1<<(i-1))) != 0;
					model->itemFromIndex(index)->setEnabled(ena);
					if(ena) {
						num++;
						last = i;
					}	
				} else {
					model->itemFromIndex(index)->setEnabled( num == 4 );
					if(num == 4) last = 0;
				}
			}
			cmbxSubslot->setCurrentIndex(last);
		} else {
			cmbxSubslot->setCurrentIndex(idxSubSlot);
		}
		cmbxSubslot->setEnabled(true);
	} else {
		cmbxSubslot->setEnabled(false);
		cmbxSubslot->setCurrentIndex(0);
	}
	subslotChanged(cmbxSubslot->currentIndex());
}

void BreakpointDialog::subslotChanged(int i)
{
	if(!currentSymbol) idxSubSlot = i;
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
