#include "SymbolManager.h"
#include "SymbolTable.h"
#include "Settings.h"
#include "Convert.h"
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>

SymbolManager::SymbolManager(SymbolTable& symtable, QWidget* parent)
	: QDialog(parent), symTable(symtable)
{
	setupUi(this);

	// restore layout
	Settings& s = Settings::get();
	restoreGeometry(s.value("SymbolManager/WindowGeometry", saveGeometry()).toByteArray());
	treeFiles->header()->restoreState(s.value("SymbolManager/HeaderFiles",
	                                  treeFiles->header()->saveState()).toByteArray());
	treeLabels->header()->restoreState(s.value("SymbolManager/HeaderSymbols",
	                                   treeLabels->header()->saveState()).toByteArray());

	treeLabelsUpdateCount = 0;
	editColumn = -1;

	// put slot checkboxes in a convenience array
	chkSlots[ 0] = chk00; chkSlots[ 1] = chk01; chkSlots[ 2] = chk02; chkSlots[ 3] = chk03;
	chkSlots[ 4] = chk10; chkSlots[ 5] = chk11; chkSlots[ 6] = chk12; chkSlots[ 7] = chk13;
	chkSlots[ 8] = chk20; chkSlots[ 9] = chk21; chkSlots[10] = chk22; chkSlots[11] = chk23;
	chkSlots[12] = chk30; chkSlots[13] = chk31; chkSlots[14] = chk32; chkSlots[15] = chk33;
	chkRegs[ 0] = chkRegA;  chkRegs[ 1] = chkRegB;  chkRegs[ 2] = chkRegC;  chkRegs[ 3] = chkRegD;
	chkRegs[ 4] = chkRegE;  chkRegs[ 5] = chkRegH;  chkRegs[ 6] = chkRegL;  chkRegs[ 7] = chkRegBC;
	chkRegs[ 8] = chkRegDE; chkRegs[ 9] = chkRegHL; chkRegs[10] = chkRegIX; chkRegs[11] = chkRegIY;
	chkRegs[12] = chkRegIXL;chkRegs[13] = chkRegIXH;chkRegs[14] = chkRegIYL;chkRegs[15] = chkRegIYH;
	chkRegs[16] = chkRegOffset; chkRegs[17] = chkRegI;

	connect(treeFiles, &QTreeWidget::itemSelectionChanged, this, &SymbolManager::fileSelectionChange);
	connect(btnAddFile,     &QPushButton::clicked, this, &SymbolManager::addFile);
	connect(btnRemoveFile,  &QPushButton::clicked, this, &SymbolManager::removeFile);
	connect(btnReloadFiles, &QPushButton::clicked, this, &SymbolManager::reloadFiles);
	connect(treeLabels, &QTreeWidget::itemSelectionChanged, this, &SymbolManager::labelSelectionChanged);
	connect(treeLabels, &QTreeWidget::itemDoubleClicked,    this, &SymbolManager::labelEdit);
	connect(treeLabels, &QTreeWidget::itemChanged,          this, &SymbolManager::labelChanged);
	connect(btnAddSymbol,    &QPushButton::clicked, this, &SymbolManager::addLabel);
	connect(btnRemoveSymbol, &QPushButton::clicked, this, &SymbolManager::removeLabel);
	connect(radJump,  &QRadioButton::toggled, this, &SymbolManager::changeType);
	connect(radVar,   &QRadioButton::toggled, this, &SymbolManager::changeType);
	connect(radValue, &QRadioButton::toggled, this, &SymbolManager::changeType);
	connect(chk00,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot00);
	connect(chk01,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot01);
	connect(chk02,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot02);
	connect(chk03,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot03);
	connect(chk10,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot10);
	connect(chk11,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot11);
	connect(chk12,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot12);
	connect(chk13,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot13);
	connect(chk20,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot20);
	connect(chk21,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot21);
	connect(chk22,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot22);
	connect(chk23,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot23);
	connect(chk30,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot30);
	connect(chk31,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot31);
	connect(chk32,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot32);
	connect(chk33,        &QCheckBox::stateChanged, this, &SymbolManager::changeSlot33);
	connect(chkRegA,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterA);
	connect(chkRegB,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterB);
	connect(chkRegC,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterC);
	connect(chkRegD,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterD);
	connect(chkRegE,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterE);
	connect(chkRegH,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterH);
	connect(chkRegL,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterL);
	connect(chkRegBC,     &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterBC);
	connect(chkRegDE,     &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterDE);
	connect(chkRegHL,     &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterHL);
	connect(chkRegIX,     &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIX);
	connect(chkRegIY,     &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIY);
	connect(chkRegIXL,    &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIXL);
	connect(chkRegIXH,    &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIXH);
	connect(chkRegIYL,    &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIYL);
	connect(chkRegIYH,    &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterIYH);
	connect(chkRegOffset, &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterOffset);
	connect(chkRegI,      &QCheckBox::stateChanged, this, &SymbolManager::changeRegisterI);

	groupSlots->setEnabled(false);
	groupSegments->setEnabled(false);
	btnRemoveFile->setEnabled(false);
	btnRemoveSymbol->setEnabled(false);

	initFileList();
	initSymbolList();
}

void SymbolManager::closeEvent(QCloseEvent* e)
{
	// store layout
	Settings& s = Settings::get();
	s.setValue("SymbolManager/WindowGeometry", saveGeometry());
	s.setValue("SymbolManager/HeaderFiles", treeFiles->header()->saveState());
	s.setValue("SymbolManager/HeaderSymbols", treeLabels->header()->saveState());
	QDialog::closeEvent(e);
}

void SymbolManager::refresh()
{
	initSymbolList();
}

/*
 * File list support functions
 */

enum TableColumns {
	FILENAME = 0,
	DESTINATION_SLOT,
	LAST_REFRESH,
};

void SymbolManager::initFileList()
{
	treeFiles->clear();
	for (int i = 0; i < symTable.symbolFilesSize(); ++i) {
		auto* item = new QTreeWidgetItem(treeFiles);
		QFileInfo info(symTable.symbolFile(i));
		item->setText(FILENAME, info.fileName());
		item->setToolTip(FILENAME, symTable.symbolFile(i));
		auto fmt = QLocale().dateTimeFormat(QLocale::ShortFormat);
		createComboBox(i);
		item->setText(LAST_REFRESH, symTable.symbolFileRefresh(i).toString(fmt));
	}
}

static const char* destinationLabels[] = {
	"All",
	"0-0", "0-1", "0-2", "0-3",
	"1-0", "1-1", "1-2", "1-3",
	"2-0", "2-1", "2-2", "2-3",
	"3-0", "3-1", "3-2", "3-3",
};

void SymbolManager::createComboBox(int row)
{
	auto* combo = new QComboBox();
	combo->setEditable(false);
	combo->insertItems(0, QStringList(std::begin(destinationLabels), std::end(destinationLabels)));
	combo->setCurrentIndex(0);

	auto* model = treeFiles->model();
	auto  index = model->index(row, DESTINATION_SLOT);

	treeFiles->setIndexWidget(index, combo);

	connect(combo, qOverload<int>(&QComboBox::currentIndexChanged),
		[this, row](int index){ addSymbolFileDestination(row, index - 1); });
}

void SymbolManager::addFile()
{
	// create dialog
	auto* d = new QFileDialog(this);
	QStringList types;
	types << "All supported files (*.omds *.sym *.map *.noi *.symbol *.publics *.sys)"
		  << "OpenMSX Debugger session files (*.omds)"
	      << "tniASM 0.x symbol files (*.sym)"
	      << "tniASM 1.x symbol files (*.sym)"
	      << "asMSX 0.x symbol files (*.sym)"
	      << "HiTech C symbol files (*.sym)"
	      << "HiTech C link map files (*.map)"
	      << "NoICE command files (*.noi)"
	      << "pasmo symbol files (*.symbol *.publics *.sys)";
	d->setNameFilters(types);
	d->setAcceptMode(QFileDialog::AcceptOpen);
	d->setFileMode(QFileDialog::ExistingFile);
	// set default directory
	d->setDirectory(Settings::get().value("SymbolManager/OpenDir", QDir::currentPath()).toString());
	// run
	if (d->exec()) {
		QString f = d->selectedNameFilter();
		QString n = d->selectedFiles().at(0);
		// load file from the correct type
		bool read = false;
		if        (f.startsWith("OpenMSX Debugger session")) {
			read = symTable.readFile(n, SymbolTable::OMDS_FILE);
		} else if (f.startsWith("tniASM 0")) {
			read = symTable.readFile(n, SymbolTable::TNIASM0_FILE);
		} else if (f.startsWith("tniASM 1")) {
			read = symTable.readFile(n, SymbolTable::TNIASM1_FILE);
		} else if (f.startsWith("asMSX")) {
			read = symTable.readFile(n, SymbolTable::ASMSX_FILE);
		} else if (f.startsWith("HiTech C symbol")) {
			read = symTable.readFile(n, SymbolTable::HTC_FILE);
		} else if (f.startsWith("HiTech C link")) {
			read = symTable.readFile(n, SymbolTable::LINKMAP_FILE);
		} else if (f.startsWith("NoICE")) {
			read = symTable.readFile(n, SymbolTable::NOICE_FILE);
		} else if (f.startsWith("pasmo")) {
			read = symTable.readFile(n, SymbolTable::PASMO_FILE);
		} else {
			read = symTable.readFile(n);
		}
		// if read succesful, add it to the list
		if (read) {
			initFileList();
			initSymbolList();
			emit symbolTableChanged();
		}
	}
	// store last used path
	Settings::get().setValue("SymbolManager/OpenDir", d->directory().absolutePath());
}

void SymbolManager::removeFile()
{
	int r = QMessageBox::question(
		this,
		tr("Remove symbol file(s)"),
		tr("When removing the symbol file(s), do you want keep or "
		   "delete the attached symbols?"),
		"Keep symbols", "Delete symbols", "Cancel", 1, 2);
	if (r == 2) return;

	for (int i = 0; i < treeFiles->selectedItems().size(); ++i) {
		symTable.unloadFile(treeFiles->selectedItems().at(i)->text(0), r == 0);
	}

	initFileList();
	initSymbolList();
	emit symbolTableChanged();
}

void SymbolManager::reloadFiles()
{
	symTable.reloadFiles();
	initFileList();
	initSymbolList();
	emit symbolTableChanged();
}

void SymbolManager::addSymbolFileDestination(int fileIndex, int slotIndex)
{
	for (Symbol* sym = symTable.findFirstAddressSymbol(0);
			sym != nullptr;
			sym = symTable.findNextAddressSymbol()) {
		if (sym->source() == &symTable.symbolFile(fileIndex)) {
			sym->setValidSlots(slotIndex == -1 ? 0xFFFF : 0xFFFF & (1 << slotIndex));
		}
	}

	refresh();
	emit symbolTableChanged();
	wipeSelectedItem();
}

void SymbolManager::fileSelectionChange()
{
	btnRemoveFile->setEnabled(!treeFiles->selectedItems().empty());
}

void SymbolManager::labelEdit(QTreeWidgetItem* item, int column)
{
	// only symbol name and value are editable
	if (column == 0 || column == 2) {
		// open editor if manually added symbol
		auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());
		if (!sym->source()) {
			// first close possible existing editor
			closeEditor();
			// open new editor
			treeLabels->openPersistentEditor(item, column);
			editItem = item;
			editColumn = column;
		}
	}
}

/*
 * Symbol support functions
 */

void SymbolManager::initSymbolList()
{
	treeLabels->clear();
	beginTreeLabelsUpdate();
	for (Symbol* sym = symTable.findFirstAddressSymbol(0);
			sym != nullptr;
			sym = symTable.findNextAddressSymbol()) {
		auto* item = new QTreeWidgetItem(treeLabels);
		// attach a pointer to the symbol object to the tree item
		item->setData(0L, Qt::UserRole, quintptr(sym));
		// update columns
		updateItemName(item);
		updateItemType(item);
		updateItemValue(item);
		updateItemSlots(item);
		updateItemSegments(item);
		updateItemRegisters(item);
	}
	endTreeLabelsUpdate();
}

void SymbolManager::beginTreeLabelsUpdate()
{
	++treeLabelsUpdateCount;
}

void SymbolManager::endTreeLabelsUpdate()
{
	if (treeLabelsUpdateCount > 0) {
		--treeLabelsUpdateCount;
	}
}

void SymbolManager::closeEditor()
{
	if (editColumn >= 0) {
		treeLabels->closePersistentEditor(editItem, editColumn);
		editColumn = -1;
	}
}

void SymbolManager::addLabel()
{
	// create an empty symbol
	auto* sym = symTable.add(std::make_unique<Symbol>(tr("New symbol"), 0));

	beginTreeLabelsUpdate();
	auto* item = new QTreeWidgetItem(treeLabels);
	item->setData(0, Qt::UserRole, quintptr(sym));
	updateItemName(item);
	updateItemType(item);
	updateItemValue(item);
	updateItemSlots(item);
	updateItemSegments(item);
	updateItemRegisters(item);
	endTreeLabelsUpdate();
	closeEditor();
	treeLabels->setFocus();
	treeLabels->setCurrentItem(item, 0);
	treeLabels->scrollToItem(item);
	treeLabels->openPersistentEditor(item, 0);
	editItem = item;
	editColumn = 0;

	// emit notification that something has changed
	emit symbolTableChanged();
}

void SymbolManager::removeLabel()
{
	QList<QTreeWidgetItem*> selection = treeLabels->selectedItems();
	// check for selection
	if (selection.empty()) return;
	// remove selected items
	bool deleted = false;
	for (auto* sel : selection) {
		// get symbol
		Symbol* sym = reinterpret_cast<Symbol*>(sel->data(0, Qt::UserRole).value<quintptr>());
		// check if symbol is from symbol file
		if (!sym->source()) {
			// remove from table
			symTable.remove(sym);
			deleted = true;
		}
	}
	if (deleted) {
		// refresh tree
		initSymbolList();
		emit symbolTableChanged();
	}
}

void SymbolManager::labelChanged(QTreeWidgetItem* item, int column)
{
	if (!treeLabelsUpdateCount) {
		auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());
		// Set symbol text from tree item if any
		QString symText = item->text(0).trimmed();
		if (symText.isEmpty()) symText = "[unnamed]";
		sym->setText(symText);
		// set value TODO: proper decoding of value
		auto value = stringToValue<uint16_t>(item->text(2));
		if (value) sym->setValue(*value);
		treeLabels->closePersistentEditor(item, column);
		editColumn = -1;
		// update item name and value
		beginTreeLabelsUpdate();
		updateItemName(item);
		updateItemValue(item);
		endTreeLabelsUpdate();
		// notify change
		emit symbolTableChanged();
	}
}

void SymbolManager::wipeSelectedItem()
{
	for (int index = 0; index < vbox0->count(); ++index) {
		static_cast<QCheckBox*>(vbox0->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < vbox1->count(); ++index) {
		static_cast<QCheckBox*>(vbox1->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < vbox2->count(); ++index) {
		static_cast<QCheckBox*>(vbox2->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < vbox3->count(); ++index) {
		static_cast<QCheckBox*>(vbox3->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < gridRegs8->count(); ++index) {
		static_cast<QCheckBox*>(gridRegs8->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < gridRegs16->count(); ++index) {
		static_cast<QCheckBox*>(gridRegs16->itemAt(index)->widget())->setChecked(false);
	}
	for (int index = 0; index < vboxSymbolType->count(); ++index) {
		auto radio = static_cast<QRadioButton*>(vboxSymbolType->itemAt(index)->widget());
		radio->setAutoExclusive(false);
		radio->setChecked(false);
		radio->setAutoExclusive(true);
	}

	txtSegments->setText("");
	treeLabels->setCurrentItem(nullptr);
}

void SymbolManager::labelSelectionChanged()
{
	// remove possible editor
	closeEditor();

	QList<QTreeWidgetItem*> selection = treeLabels->selectedItems();
	// check if is available at all
	if (selection.empty()) {
		wipeSelectedItem();
		// disable everything
		btnRemoveSymbol->setEnabled(false);
		groupSlots->setEnabled(false);
		groupSegments->setEnabled(false);
		groupType->setEnabled(false);
		groupRegs8->setEnabled(false);
		groupRegs16->setEnabled(false);
		return;
	}
	// check selection for "manual insertion", identical slot mask,
	// identical register mask, identical types and value size
	bool removeButActive = true;
	bool anyEight = false;
	bool sameType = true;
	Symbol::SymbolType type;
	int slotMask, slotMaskMultiple = 0;
	int regMask, regMaskMultiple = 0;
	for (auto selit = selection.begin(); selit != selection.end(); ++selit) {
		// get symbol
		auto* sym = reinterpret_cast<Symbol*>((*selit)->data(0, Qt::UserRole).value<quintptr>());
		// check if symbol is from symbol file
		if (sym->source()) removeButActive = false;

		if (selit == selection.begin()) {
			// first item, reference for slotMask and regMask
			slotMask = sym->validSlots();
			regMask = sym->validRegisters();
			type = sym->type();
		} else {
			// other, set all different bits
			slotMaskMultiple |= slotMask ^ sym->validSlots();
			regMaskMultiple |= regMask ^ sym->validRegisters();
			// check for different type
			if (type != sym->type()) sameType = false;
		}

		// check for 8 bit values
		if ((sym->value() & 0xFF00) == 0) {
			anyEight = true;
		}
	}

	btnRemoveSymbol->setEnabled(removeButActive);
	groupSlots->setEnabled(true);
	groupType->setEnabled(true);
	groupRegs8->setEnabled(anyEight);
	groupRegs16->setEnabled(true);
	beginTreeLabelsUpdate();

	// set slot selection
	for (auto* chkSlot : chkSlots) {
		chkSlot->setTristate(false);
		if (slotMaskMultiple & 1) {
			chkSlot->setCheckState(Qt::PartiallyChecked);
		} else if (slotMask & 1) {
			chkSlot->setCheckState(Qt::Checked);
		} else {
			chkSlot->setCheckState(Qt::Unchecked);
		}
		slotMask >>= 1;
		slotMaskMultiple >>= 1;
	}
	// set register selection
	for (auto* chkReg : chkRegs) {
		chkReg->setTristate(false);
		if (regMaskMultiple & 1) {
			chkReg->setCheckState(Qt::PartiallyChecked);
		} else if (regMask & 1) {
			chkReg->setCheckState(Qt::Checked);
		} else {
			chkReg->setCheckState(Qt::Unchecked);
		}
		regMask >>= 1;
		regMaskMultiple >>= 1;
	}

	// temporarily disable exclusive radiobuttons to be able to
	// deselect them all.
	radJump->setAutoExclusive (false);
	radVar->setAutoExclusive  (false);
	radValue->setAutoExclusive(false);
	// set type radio buttons
	radJump->setChecked (sameType && type == Symbol::JUMPLABEL);
	radVar->setChecked  (sameType && type == Symbol::VARIABLELABEL);
	radValue->setChecked(sameType && type == Symbol::VALUE);
	// enable exclusive radiobuttons (this won't immediately activate
	// one if none are active).
	radJump->setAutoExclusive (true);
	radVar->setAutoExclusive  (true);
	radValue->setAutoExclusive(true);
	endTreeLabelsUpdate();
}

void SymbolManager::changeSlot(int id, int state)
{
	if (treeLabelsUpdateCount) return;

	// disallow another tristate selection
	chkSlots[id]->setTristate(false);
	// get selected items
	QList<QTreeWidgetItem*> selection = treeLabels->selectedItems();

	// update items
	beginTreeLabelsUpdate();
	int bit = 1 << id;
	for (auto* sel : selection) {
		auto* sym = reinterpret_cast<Symbol*>(sel->data(0, Qt::UserRole).value<quintptr>());
		// set or clear bit
		if (state == Qt::Checked) {
			sym->setValidSlots(sym->validSlots() |  bit);
		} else {
			sym->setValidSlots(sym->validSlots() & ~bit);
		}
		// update item in treewidget
		updateItemSlots(sel);
	}
	endTreeLabelsUpdate();
	// notify change
	emit symbolTableChanged();
}

void SymbolManager::changeRegister(int id, int state)
{
	if (treeLabelsUpdateCount) return;

	// disallow another tristate selection
	chkRegs[id]->setTristate(false);
	// get selected items
	QList<QTreeWidgetItem*> selection = treeLabels->selectedItems();

	// update items
	beginTreeLabelsUpdate();
	int bit = 1 << id;
	for (auto* sel : selection) {
		auto* sym = reinterpret_cast<Symbol*>(sel->data(0, Qt::UserRole).value<quintptr>());
		// set or clear bit
		if (state == Qt::Checked) {
			sym->setValidRegisters(sym->validRegisters() |  bit);
		} else {
			sym->setValidRegisters(sym->validRegisters() & ~bit);
		}
		// update item in treewidget
		updateItemRegisters(sel);
	}
	endTreeLabelsUpdate();
	// notify change
	emit symbolTableChanged();
}

void SymbolManager::changeType(bool /*checked*/)
{
	if (treeLabelsUpdateCount) return;

	// determine selected type
	Symbol::SymbolType newType = Symbol::JUMPLABEL;
	if (radVar->isChecked()) {
		newType = Symbol::VARIABLELABEL;
	} else if (radValue->isChecked()) {
		newType = Symbol::VALUE;
	}

	// get selected items
	QList<QTreeWidgetItem*> selection = treeLabels->selectedItems();

	// update items
	beginTreeLabelsUpdate();
	for (auto* sel : selection) {
		auto* sym = reinterpret_cast<Symbol*>(sel->data(0, Qt::UserRole).value<quintptr>());
		sym->setType(newType);
		updateItemType(sel);
	}
	endTreeLabelsUpdate();
	// notify change
	emit symbolTableChanged();
}


/*
 * Symbol tree layout functions
 */

void SymbolManager::updateItemName(QTreeWidgetItem* item)
{
	auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());
	// set text and icon
	item->setText(0, sym->text());
	if (sym->source()) {
		item->setIcon(0, QIcon(":/icons/symfil.png"));
		item->setText(6, *sym->source());
	} else {
		item->setIcon(0, QIcon(":/icons/symman.png"));
	}
	// set color based on status as well as status in 2nd column
	switch (sym->status()) {
	case Symbol::HIDDEN:
		item->setForeground(0, QColor(128, 128, 128));
		break;
	case Symbol::LOST:
		item->setForeground(0, QColor(128, 0, 0));
		break;
	default:
		break;
	}
}

void SymbolManager::updateItemType(QTreeWidgetItem* item)
{
	auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());
	// set symbol type in 2nd column
	switch (sym->type()) {
	case Symbol::JUMPLABEL:
		item->setText(1, tr("Jump label"));
		break;
	case Symbol::VARIABLELABEL:
		item->setText(1, tr("Variable label"));
		break;
	case Symbol::VALUE:
		item->setText(1, tr("Value"));
		break;
	}
}

void SymbolManager::updateItemValue(QTreeWidgetItem* item)
{
	auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());

	// symbol value in 3rd column
	item->setText(2, hexValue(sym->value(), 4));
}

void SymbolManager::updateItemSlots(QTreeWidgetItem* item)
{
	auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());

	QString slotText;
	int slotmask = sym->validSlots();
	// value represents 16 bits for 4 subslots in 4 slots
	if (slotmask == 0xFFFF) {
		slotText = tr("All");
	} else if (slotmask == 0) {
		slotText = tr("None");
	} else {
		// create a list of valid slots
		// loop over all primary slots
		for (int ps = 0; ps < 4; ++ps) {
			QString subText;
			int subslots = (slotmask >> (4 * ps)) & 15;
			if (subslots == 15) {
				// all subslots are ok
				subText = QString("%1-*").arg(ps);
			} else if (subslots) {
				// some subslots are ok
				if (subslots & 1) subText += "/0";
				if (subslots & 2) subText += "/1";
				if (subslots & 4) subText += "/2";
				if (subslots & 8) subText += "/3";
				subText = QString("%1-").arg(ps) + subText.mid(1);
			}
			// add to string if any subslots were ok
			if (!subText.isEmpty()) {
				if (!slotText.isEmpty()) slotText += ", ";
				slotText += subText;
			}
		}
	}

	// valid slots in 4th column
	item->setText(3, slotText);
}

void SymbolManager::updateItemSegments(QTreeWidgetItem* item)
{
	item->data(0, Qt::UserRole).value<quintptr>();
}

void SymbolManager::updateItemRegisters(QTreeWidgetItem* item)
{
	auto* sym = reinterpret_cast<Symbol*>(item->data(0, Qt::UserRole).value<quintptr>());

	QString regText;
	int regmask = sym->validRegisters();
	// value represents 16 bits for 4 subslots in 4 slots
	if (regmask == 0x3FFFF) {
		regText = tr("All");
	} else if (regmask == 0) {
		regText = tr("None");
	} else {
		if ((regmask & Symbol::REG_ALL8) == Symbol::REG_ALL8) {
			// all 8 bit registers selected
			regText = "All 8 bit, ";
			regmask ^= Symbol::REG_ALL8;
		} else if ((regmask & Symbol::REG_ALL16) == Symbol::REG_ALL16) {
			// all 16 bit registers selected
			regText = "All 16 bit, ";
			regmask ^= Symbol::REG_ALL16;
		}
		// register list for remaining registers
		static const char* const registers[] = {
			"A", "B", "C", "D", "E", "H", "L", "BC", "DE", "HL",
		        "IX", "IY", "IXL", "IXH", "IYL", "IYH", "Offset", "I"
		};
		for (const char* reg : registers) {
			if (regmask & 1) {
				regText += QString("%1, ").arg(reg);
			}
			regmask >>= 1;
		}
		regText.chop(2);
	}

	// valid slots in 4th column
	item->setText(5, regText);
}


// load of functions that shouldn't really be necessary
void SymbolManager::changeSlot00(int state)
{
	changeSlot(0, state);
}
void SymbolManager::changeSlot01(int state)
{
	changeSlot(1, state);
}
void SymbolManager::changeSlot02(int state)
{
	changeSlot(2, state);
}
void SymbolManager::changeSlot03(int state)
{
	changeSlot(3, state);
}
void SymbolManager::changeSlot10(int state)
{
	changeSlot(4, state);
}
void SymbolManager::changeSlot11(int state)
{
	changeSlot(5, state);
}
void SymbolManager::changeSlot12(int state)
{
	changeSlot(6, state);
}
void SymbolManager::changeSlot13(int state)
{
	changeSlot(7, state);
}
void SymbolManager::changeSlot20(int state)
{
	changeSlot(8, state);
}
void SymbolManager::changeSlot21(int state)
{
	changeSlot(9, state);
}
void SymbolManager::changeSlot22(int state)
{
	changeSlot(10, state);
}
void SymbolManager::changeSlot23(int state)
{
	changeSlot(11, state);
}
void SymbolManager::changeSlot30(int state)
{
	changeSlot(12, state);
}
void SymbolManager::changeSlot31(int state)
{
	changeSlot(13, state);
}
void SymbolManager::changeSlot32(int state)
{
	changeSlot(14, state);
}
void SymbolManager::changeSlot33(int state)
{
	changeSlot(15, state);
}
void SymbolManager::changeRegisterA(int state)
{
	changeRegister(0, state);
}
void SymbolManager::changeRegisterB(int state)
{
	changeRegister(1, state);
}
void SymbolManager::changeRegisterC(int state)
{
	changeRegister(2, state);
}
void SymbolManager::changeRegisterD(int state)
{
	changeRegister(3, state);
}
void SymbolManager::changeRegisterE(int state)
{
	changeRegister(4, state);
}
void SymbolManager::changeRegisterH(int state)
{
	changeRegister(5, state);
}
void SymbolManager::changeRegisterL(int state)
{
	changeRegister(6, state);
}
void SymbolManager::changeRegisterBC(int state)
{
	changeRegister(7, state);
}
void SymbolManager::changeRegisterDE(int state)
{
	changeRegister(8, state);
}
void SymbolManager::changeRegisterHL(int state)
{
	changeRegister(9, state);
}
void SymbolManager::changeRegisterIX(int state)
{
	changeRegister(10, state);
}
void SymbolManager::changeRegisterIY(int state)
{
	changeRegister(11, state);
}
void SymbolManager::changeRegisterIXL(int state)
{
	changeRegister(12, state);
}
void SymbolManager::changeRegisterIXH(int state)
{
	changeRegister(13, state);
}
void SymbolManager::changeRegisterIYL(int state)
{
	changeRegister(14, state);
}
void SymbolManager::changeRegisterIYH(int state)
{
	changeRegister(15, state);
}
void SymbolManager::changeRegisterOffset(int state)
{
	changeRegister(16, state);
}
void SymbolManager::changeRegisterI(int state)
{
	changeRegister(17, state);
}
