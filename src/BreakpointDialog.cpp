#include "BreakpointDialog.h"
#include "DebugSession.h"
#include "Convert.h"
#include <QStandardItemModel>
#include <memory>

BreakpointDialog::BreakpointDialog(const MemoryLayout& ml, DebugSession* session, QWidget* parent)
	: QDialog(parent), memLayout(ml), currentSymbol(nullptr)
{
	setupUi(this);

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

	connect(edtAddress, &QLineEdit::textEdited, this, &BreakpointDialog::addressChanged);
	connect(edtAddressRange, &QLineEdit::textEdited, this, &BreakpointDialog::addressChanged);
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

std::optional<uint16_t> BreakpointDialog::parseInput(const QLineEdit& ed, Symbol** symbol) const {
	if (auto value = stringToValue<uint16_t>(ed.text())) {
		return value;
	}

	// convert symbol to value
	if (debugSession) {
		if (Symbol* s = debugSession->symbolTable().getAddressSymbol(ed.text())) {
			if (symbol) *symbol = s;
			return s->value();
		}
	}

	return {};
}

std::optional<AddressRange> BreakpointDialog::addressRange(Symbol** symbol) const
{
	if (auto address = parseInput(*edtAddress, symbol)) {
		if (edtAddressRange->text().isEmpty()) {
			return AddressRange{*address};
		} else if (auto endAddress = parseInput(*edtAddressRange)) {
			if (*address <= *endAddress) {
				return AddressRange{*address, make_if(*address != *endAddress, *endAddress)};
			}
		}
	}
	return {};
}

Slot BreakpointDialog::slot() const
{
	auto slot = make_positive_optional<uint8_t>(cmbxSlot->currentIndex() - 1);
	auto subslot = make_positive_optional<uint8_t>(cmbxSubslot->currentIndex() - 1);
	return Slot{slot, subslot};
}

std::optional<uint8_t> BreakpointDialog::segment() const
{
	return make_positive_optional<uint8_t>(cmbxSegment->currentIndex() - 1);
}

QString BreakpointDialog::condition() const
{
	return (cbCondition->checkState() == Qt::Checked) ? txtCondition->toPlainText() : "";
}

void BreakpointDialog::setData(Breakpoint::Type type, std::optional<AddressRange> range, Slot slot,
                               std::optional<uint8_t> segment, QString condition)
{
	// set type
	cmbxType->setCurrentIndex(int(type));
	typeChanged(cmbxType->currentIndex());

	// set address
	edtAddress->setText(range ? hexValue(range->start): "");
	if (range) {
		addressChanged(edtAddress->text());
	}

	// set end address
	if (edtAddressRange->isEnabled()) {
		edtAddressRange->setText(range && range->end ? hexValue(*range->end) : "");
		addressChanged(edtAddressRange->text());
	}

	// set primary slot
	if (cmbxSlot->isEnabled()) {
		cmbxSlot->setCurrentIndex(slot.ps ? *slot.ps + 1 : -1);
		slotChanged(cmbxSlot->currentIndex());
	}

	// set secondary slot
	if (cmbxSubslot->isEnabled()) {
		cmbxSubslot->setCurrentIndex(slot.ss ? *slot.ss + 1 : -1);
		subslotChanged(cmbxSubslot->currentIndex());
	}

	// set segment
	if (cmbxSegment->isEnabled()) {
		cmbxSegment->setCurrentIndex(segment ? *segment + 1 : -1);
	}

	// set condition
	condition = condition.trimmed();
	if (cbCondition->isChecked() == condition.isEmpty())
		cbCondition->setChecked(!condition.isEmpty());
	txtCondition->setText(condition);
}

void BreakpointDialog::disableSlots()
{
	cmbxSlot->setCurrentIndex(0);
	cmbxSlot->setEnabled(false);
}

void BreakpointDialog::enableSlots()
{
	auto* model = qobject_cast<QStandardItemModel*>(cmbxSlot->model());
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

void BreakpointDialog::addressChanged(const QString& /*text*/)
{
	// adjust controls for (in)correct values
	auto range = addressRange(&currentSymbol);
	QPalette pal;

	if (!range) {
		// set line edit text to red for invalid values TODO: make configurable
		pal.setColor(QPalette::Normal, QPalette::Text, Qt::red);
		disableSlots();
	} else {
		enableSlots();
		slotChanged(cmbxSlot->currentIndex());
	}

	edtAddress->setPalette(pal);
	edtAddressRange->setPalette(pal);
	okButton->setEnabled(range.has_value());
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
			int v = (currentSymbol->validSlots() >> (4 * (s - 1))) & 15;
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
	int oldseg = cmbxSegment->currentIndex() - 1;
	cmbxSegment->clear();
	cmbxSegment->addItem("any");

	int ps = cmbxSlot->currentIndex() - 1;
	int ss = i - 1;

	if (ps >= 0 && !memLayout.isSubslotted[ps]) ss = 0;

	int mapsize;
	if (ps < 0 || ss < 0 || memLayout.mapperSize[ps][ss] == 0) {
		auto range = addressRange();

		// no memory mapper, maybe rom mapper?
		if (range && memLayout.romBlock[((range->start & 0xE000) >> 13)] >= 0) {
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
	if (oldseg >= 0 && oldseg < mapsize) {
		cmbxSegment->setCurrentIndex(oldseg + 1);
	}
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
