#include "BreakpointDialog.h"
#include "DebugSession.h"
#include "Convert.h"
#include <QStandardItemModel>
#include <memory>

BreakpointDialog::BreakpointDialog(const MemoryLayout& ml, DebugSession* session, QWidget* parent)
	: QDialog(parent), memLayout(ml), currentSymbol(nullptr)
{
	setupUi(this);

	value = valueEnd = -1;

	debugSession = session;
	if (session) {
		// create address completer
		jumpCompleter = std::make_unique<QCompleter>(session->symbolTable().labelList(), this);
		allCompleter = std::make_unique<QCompleter>(session->symbolTable().labelList(true), this);
		jumpCompleter->setCaseSensitivity(Qt::CaseInsensitive);
		allCompleter->setCaseSensitivity(Qt::CaseInsensitive);
		connect(jumpCompleter.get(), SIGNAL(activated(const QString&)), this, SLOT(addressChanged(const QString&)));
		connect(allCompleter.get(),  SIGNAL(activated(const QString&)), this, SLOT(addressChanged(const QString&)));
	}

	connect(edtAddress,      SIGNAL(textEdited(const QString&)), this, SLOT(addressChanged(const QString&)));
	connect(edtAddressRange, SIGNAL(textEdited(const QString&)), this, SLOT(addressChanged(const QString&)));
	connect(cmbxType,    SIGNAL(activated(int)), this, SLOT(typeChanged(int)));
	connect(cmbxSlot,    SIGNAL(activated(int)), this, SLOT(slotChanged(int)));
	connect(cmbxSubslot, SIGNAL(activated(int)), this, SLOT(subslotChanged(int)));
	connect(cbCondition, SIGNAL(stateChanged(int)), this, SLOT(hasCondition(int)));

	typeChanged(0);
	slotChanged(0);
	idxSlot = idxSubSlot = 0;
	hasCondition(0);
}

Breakpoint::Type BreakpointDialog::type() const
{
	return Breakpoint::Type(cmbxType->currentIndex());
}

int BreakpointDialog::address() const
{
	return value;
}

int BreakpointDialog::addressEndRange() const
{
	return (valueEnd < value) ? value : valueEnd;
}

int BreakpointDialog::slot() const
{
	return cmbxSlot->currentIndex() - 1;
}

int BreakpointDialog::subslot() const
{
	return cmbxSubslot->currentIndex() - 1;
}

int BreakpointDialog::segment() const
{
	return cmbxSegment->currentIndex() - 1;
}

QString BreakpointDialog::condition() const
{
	return (cbCondition->checkState() == Qt::Checked) ? txtCondition->toPlainText() : "";
}

void BreakpointDialog::setData(Breakpoint::Type type, int address, qint8 ps, qint8 ss,
                               qint16 segment, int addressEnd, QString condition)
{
	// set type
	cmbxType->setCurrentIndex(int(type));
	typeChanged(cmbxType->currentIndex());

	// set address
	if (address >= 0) {
		edtAddress->setText(hexValue(address));
		addressChanged(edtAddress->text());
	}

	// primary slot
	if (cmbxSlot->isEnabled() && ps >= 0) {
		cmbxSlot->setCurrentIndex(ps + 1);
		slotChanged(cmbxSlot->currentIndex());
	}

	// secondary slot
	if (cmbxSubslot->isEnabled() && ss >= 0) {
		cmbxSubslot->setCurrentIndex(ss + 1);
		subslotChanged(cmbxSubslot->currentIndex());
	}

	// segment
	if (cmbxSegment->isEnabled() && segment >= 0) {
		cmbxSegment->setCurrentIndex(segment + 1);
	}

	// end address
	if (edtAddressRange->isEnabled() && addressEnd >= 0) {
		edtAddressRange->setText(hexValue(addressEnd));
		addressChanged(edtAddressRange->text());
	}

	// condition
	condition = condition.trimmed();
	if (cbCondition->isChecked() == condition.isEmpty())
		cbCondition->setChecked(!condition.isEmpty());
	txtCondition->setText(condition);
}

void BreakpointDialog::addressChanged(const QString& text)
{
	// determine source
	QLineEdit *ed;
	if (text == edtAddress->text())
		ed = edtAddress;
	else
		ed = edtAddressRange;
	bool first = ed == edtAddress;

	// convert value
	int v = stringToValue(text);
	if (v == -1 && debugSession) {
		Symbol *s = debugSession->symbolTable().getAddressSymbol(text);
		if (s) v = s->value();
		if (first) currentSymbol = s;
	}

	// assign address
	if (first)
		value = v;
	else {
		if (v > value)
			valueEnd = v;
		else {
			valueEnd = -1;
			v = -1;
		}
	}

	// adjust controls for (in)correct values
	auto* model = qobject_cast<QStandardItemModel*>(cmbxSlot->model());
	QPalette pal;
	if (v == -1) {
		// set line edit text to red for invalid values TODO: make configurable
		pal.setColor(QPalette::Normal, QPalette::Text, Qt::red);
		ed->setPalette(pal);
		if (first) {
			cmbxSlot->setCurrentIndex(0);
			cmbxSlot->setEnabled(false);
		}
	} else {
		//pal.setColor(QPalette::Normal, QPalette::Text, Qt::black);
		ed->setPalette(pal);

		if (!first) return;

		cmbxSlot->setEnabled(true);

		if (currentSymbol) {
			// enable allowed slots
			int s = currentSymbol->validSlots();
			int num = 0;
			int last = 0;
			for (int i = 4; i >= 0; i--) {
				QModelIndex index = model->index(i, 0, cmbxSlot->rootModelIndex());
				if (i) {
					bool ena = (s & (15 << (4 * (i - 1)))) != 0;
					model->itemFromIndex(index)->setEnabled(ena);
					if (ena) {
						num++;
						last = i;
					}
				} else {
					model->itemFromIndex(index)->setEnabled(num == 4);
					if (num == 4) last = 0;
				}
			}
			cmbxSlot->setCurrentIndex(last);
		} else {
			// enable everything
			for (int i = 0; i < 5; i++) {
				QModelIndex index = model->index(i, 0, cmbxSlot->rootModelIndex());
				model->itemFromIndex(index)->setEnabled(true);
			}
			cmbxSlot->setCurrentIndex(idxSlot);
		}
	}
	slotChanged(cmbxSlot->currentIndex());
}

void BreakpointDialog::typeChanged(int s)
{
	switch(s) {
		case 1:
		case 2:
			lblAddress->setText(tr("Add watchpoint at memory address or range:"));
			edtAddressRange->setVisible(true);
			edtAddress->setCompleter(allCompleter.get());
			edtAddressRange->setCompleter(allCompleter.get());
			break;
		case 3:
		case 4:
			lblAddress->setText(tr("Add watchpoint on IO port or range:"));
			edtAddressRange->setVisible(true);
			edtAddress->setCompleter(nullptr);
			edtAddressRange->setCompleter(nullptr);
			break;
		default:
			lblAddress->setText(tr("Add breakpoint at address:"));
			edtAddressRange->setVisible(false);
			edtAddress->setCompleter(jumpCompleter.get());
	}

	switch(s) {
		case 1:
		case 2:
			lblInSlot->setText(tr("With address in"));
			break;
		default:
			lblInSlot->setText(tr("With PC in"));
	}

	if (s == 5) {
		lblAddress->setEnabled(false);
		edtAddress->setEnabled(false);
		lblSlot->setEnabled(false);
		lblSubslot->setEnabled(false);
		lblSegment->setEnabled(false);
		cmbxSlot->setEnabled(false);
		cmbxSubslot->setEnabled(false);
		cmbxSegment->setEnabled(false);
		cbCondition->setCheckState(Qt::Checked);
		cbCondition->setEnabled(false);
	} else {
		lblAddress->setEnabled(true);
		edtAddress->setEnabled(true);
		lblSlot->setEnabled(true);
		lblSubslot->setEnabled(true);
		lblSegment->setEnabled(true);
		addressChanged(edtAddress->text());
		cbCondition->setEnabled(true);
		cbCondition->setCheckState(txtCondition->toPlainText().isEmpty() ? Qt::Unchecked : Qt::Checked);
	}
}

void BreakpointDialog::slotChanged(int s)
{
	if (!currentSymbol) idxSlot = s;
	if (s && memLayout.isSubslotted[s - 1]) {
		if (currentSymbol) {
			// enable subslots
			int v = (currentSymbol->validSlots() >> (4*(s-1))) & 15;
			int num = 0;
			int last = 0;
			auto* model = qobject_cast<QStandardItemModel*>(cmbxSubslot->model());
			for (int i = 4; i >= 0; i--) {
				QModelIndex index = model->index(i, 0, cmbxSubslot->rootModelIndex());
				if (i) {
					bool ena = (v & (1 << (i - 1))) != 0;
					model->itemFromIndex(index)->setEnabled(ena);
					if (ena) {
						num++;
						last = i;
					}
				} else {
					model->itemFromIndex(index)->setEnabled(num == 4);
					if (num == 4) last = 0;
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
	if (!currentSymbol) idxSubSlot = i;
	int oldseg = cmbxSegment->currentIndex()-1;
	cmbxSegment->clear();
	cmbxSegment->addItem("any");

	int ps = cmbxSlot->currentIndex() - 1;
	int ss = i - 1;

	if (ps >=0 && !memLayout.isSubslotted[ps]) ss = 0;

	int mapsize;
	if (ps < 0 || ss < 0 || memLayout.mapperSize[ps][ss] == 0) {
		// no memory mapper, maybe rom mapper?
		if (value >= 0 && memLayout.romBlock[((value & 0xE000)>>13)] >= 0) {
			mapsize = 256; // TODO: determine real size
		} else {
			cmbxSegment->setEnabled(false);
			cmbxSegment->setCurrentIndex(0);
			return;
		}
	} else
		mapsize = memLayout.mapperSize[ps][ss];

	for (int s = 0; s < mapsize; ++s) {
		cmbxSegment->addItem(QString("%1").arg(s));
	}
	cmbxSegment->setEnabled(true);
	if (oldseg >= 0 && oldseg < mapsize)
		cmbxSegment->setCurrentIndex(oldseg+1);
}

void BreakpointDialog::hasCondition(int state)
{
	if (state == Qt::Checked) {
		txtCondition->setVisible(true);
		layout()->setSizeConstraint(QLayout::SetDefaultConstraint);
		resize(width(), conditionHeight);
	} else {
		conditionHeight = height();
		txtCondition->setVisible(false);
		resize(width(), sizeHint().height());
		layout()->setSizeConstraint(QLayout::SetMaximumSize);
	}
}
