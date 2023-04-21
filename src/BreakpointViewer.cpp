#include "BreakpointViewer.h"
#include "Convert.h"
#include "CommClient.h"
#include "DebugSession.h"
#include "OpenMSXConnection.h"
#include "ScopedAssign.h"
#include "ranges.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QComboBox>
#include <cassert>

static constexpr int UNDEFINED_ROW = -1;

static int newTableItem = 0;

enum TableColumns {
	ENABLED = 0,
	WP_TYPE = 1,
	LOCATION = 2,
	WP_REGION = 2,
	T_CONDITION = 3,
	SLOT = 4,
	SEGMENT = 5,
	BREAKPOINT_ID = 6,
	TABLE_INDEX = 7,
};

static QString locationString(const AddressRange& range, int adrLen)
{
	return QString("%1%2%3")
	       .arg(hexValue(range.start, adrLen))
	       .arg(range.end ? ":" : "")
	       .arg(range.end ? hexValue(*range.end, adrLen) : "");
}

BreakpointViewer::BreakpointViewer(DebugSession& session, QWidget* parent)
	:  QTabWidget(parent),
	  ui(new Ui::BreakpointViewer),
	  debugSession(session)
{
	setupUi(this);

	connect(btnAddBp,    &QPushButton::clicked, this, &BreakpointViewer::on_btnAddBp_clicked);
	connect(btnRemoveBp, &QPushButton::clicked, this, &BreakpointViewer::on_btnRemoveBp_clicked);
	connect(btnAddWp,    &QPushButton::clicked, this, &BreakpointViewer::on_btnAddWp_clicked);
	connect(btnRemoveWp, &QPushButton::clicked, this, &BreakpointViewer::on_btnRemoveWp_clicked);
	connect(btnAddCn,    &QPushButton::clicked, this, &BreakpointViewer::on_btnAddCn_clicked);
	connect(btnRemoveCn, &QPushButton::clicked, this, &BreakpointViewer::on_btnRemoveCn_clicked);

	bpTableWidget->horizontalHeader()->setHighlightSections(false);
	bpTableWidget->sortByColumn(LOCATION, Qt::AscendingOrder);
	bpTableWidget->setColumnHidden(WP_TYPE, true);
	bpTableWidget->setColumnHidden(BREAKPOINT_ID, true);
	bpTableWidget->setColumnHidden(TABLE_INDEX, true);
	bpTableWidget->resizeColumnsToContents();
	bpTableWidget->setSortingEnabled(true);
	connect(bpTableWidget, &QTableWidget::itemPressed, this, &BreakpointViewer::on_itemPressed);
	connect(bpTableWidget, &QTableWidget::itemChanged, this, &BreakpointViewer::changeBpTableItem);
	connect(bpTableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this,
	        &BreakpointViewer::on_headerClicked);

	wpTableWidget->horizontalHeader()->setHighlightSections(false);
	wpTableWidget->setColumnHidden(BREAKPOINT_ID, true);
	wpTableWidget->setColumnHidden(TABLE_INDEX, true);
	wpTableWidget->sortByColumn(WP_REGION, Qt::AscendingOrder);
	wpTableWidget->resizeColumnsToContents();
	wpTableWidget->setSortingEnabled(true);
	connect(wpTableWidget, &QTableWidget::itemChanged, this,
	        &BreakpointViewer::changeWpTableItem);

	cnTableWidget->horizontalHeader()->setHighlightSections(false);
	cnTableWidget->setColumnHidden(WP_TYPE, true);
	cnTableWidget->setColumnHidden(BREAKPOINT_ID, true);
	cnTableWidget->setColumnHidden(LOCATION, true);
	cnTableWidget->setColumnHidden(SLOT, true);
	cnTableWidget->setColumnHidden(SEGMENT, true);
	cnTableWidget->setColumnHidden(TABLE_INDEX, true);
	cnTableWidget->sortByColumn(T_CONDITION, Qt::AscendingOrder);
	cnTableWidget->resizeColumnsToContents();
	cnTableWidget->setSortingEnabled(true);
	connect(cnTableWidget, &QTableWidget::itemChanged, this,
	        &BreakpointViewer::changeCnTableItem);

	tables[BreakpointRef::BREAKPOINT] = bpTableWidget;
	tables[BreakpointRef::WATCHPOINT] = wpTableWidget;
	tables[BreakpointRef::CONDITION]  = cnTableWidget;
}

std::optional<int> BreakpointViewer::getTableIndexByRow(BreakpointRef::Type type, int row) const
{
	auto* table = tables[type];
	if (row >= table->rowCount()) return {};
	auto* item = table->item(row, TABLE_INDEX);
	return item->text().toInt();
}

// TODO: move the createSetCommand to a session manager
void BreakpointViewer::createBreakpoint(BreakpointRef::Type type, int row)
{
	if (type == BreakpointRef::CONDITION) _createCondition(row);
	else _createBreakpoint(type, row);
}

void BreakpointViewer::_handleSyncError(const QString& error)
{
	int choice = QMessageBox::warning(nullptr, tr("Synchronization error"),
			tr("Error message from openMSX:\n\"%1\"\n\nClick on Reset to resynchronize with openMSX")
			.arg(error.trimmed()), QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		emit contentsUpdated();
	}
}

void BreakpointViewer::_handleKeyAlreadyExists()
{
	int choice = QMessageBox::warning(nullptr, tr("Reference error"),
			tr("A breakpoint with the same name already exists locally. This is probably "
			   "a synchronization problem.\n\nClick on Reset to resynchronize with openMSX"),
			QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		emit contentsUpdated();
	}
}

void BreakpointViewer::_handleKeyNotFound()
{
	int choice = QMessageBox::warning(nullptr, tr("Reference error"),
			tr("Breakpoint was not found locally. This is probably a synchronization problem.\n\n"
			   "Click on Reset to resynchronize with openMSX"),
			QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		emit contentsUpdated();
	}
}

void BreakpointViewer::_createBreakpoint(BreakpointRef::Type type, int row)
{
	assert(type != BreakpointRef::CONDITION);
	auto* table = tables[type];
	auto* model = table->model();
	auto* combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
	Breakpoint::Type wtype = type == BreakpointRef::WATCHPOINT ? readComboBox(row) : Breakpoint::BREAKPOINT;

	QString location = table->item(row, LOCATION)->text();
	auto range = parseLocationField({}, type, location, combo ? combo->currentText() : "");
	setBreakpointChecked(type, row, range ? Qt::Checked : Qt::Unchecked);
	if (!range) return;

	QString condition = table->item(row, T_CONDITION)->text();
	auto slot = parseSlotField({}, table->item(row, SLOT)->text());
	auto segment = parseSegmentField({}, table->item(row, SEGMENT)->text());
	const QString cmdStr = Breakpoints::createSetCommand(wtype, range, slot, segment, condition);

	auto* command = new Command(cmdStr,
		[=] (const QString& id) {
			auto tableIndex = getTableIndexByRow(type, row);
			assert(tableIndex);

			// remove old BreakpointRef
			if (auto* ref = findBreakpointRef(type, row)) {
				maps[type].erase(ref->id);
			}

			// ref doesn't exist, so it must be a new breakpoint
			BreakpointRef newRef = {type, id, *tableIndex, -1};
			addBreakpointRef(id, newRef);

			// update breakpoint id
			setTextField(type, row, BREAKPOINT_ID, id);

			disableRefresh = false;
			emit contentsUpdated();
		},
		[this] (const QString& error) {
			disableRefresh = false;
			_handleSyncError(error);
		}
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::replaceBreakpoint(BreakpointRef::Type type, int row)
{
	auto* table = tables[type];
	auto* item  = table->item(row, BREAKPOINT_ID);
	QString id  = item->text();
	// remove and create breakpoint without calling refresh in between.
	disableRefresh = true;

	// replacing is the same as removing an old breakpoint and then create a new one
	const QString cmdStr = breakpoints->createRemoveCommand(id);
	auto* command = new Command(cmdStr,
		[this, type, row] (const QString& /*result*/) {
			createBreakpoint(type, row);
		},
		[this](const QString& error) {
			// restore default behaviour on error
			disableRefresh = false;
			_handleSyncError(error);
		}
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::removeBreakpoint(BreakpointRef::Type type, int row, bool removeLocal)
{
	auto* table = tables[type];

	auto* item  = table->item(row, BREAKPOINT_ID);
	assert(!item->text().isEmpty());
	QString id  = item->text();
	const QString cmdStr = Breakpoints::createRemoveCommand(id);

	auto* command = new Command(cmdStr,
		[=] (const QString& /*result*/) {
			size_t erased = maps[type].erase(id);

			if (erased != 1) {
				setBreakpointChecked(type, row, Qt::Unchecked);
				_handleKeyNotFound();
				return;
			}

			if (removeLocal) {
				auto* table = tables[type];
				auto     sa = ScopedAssign(userMode, false);
				table->removeRow(row);
			}

			emit contentsUpdated();
		},
		[this] (const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createSetCommand to a session manager
void BreakpointViewer::_createCondition(int row)
{
	BreakpointRef::Type type = BreakpointRef::CONDITION;
	QString condition = cnTableWidget->item(row, T_CONDITION)->text();
	if (condition.isEmpty()) {
		setBreakpointChecked(BreakpointRef::CONDITION, row, Qt::Unchecked);
		return;
	} else {
		setBreakpointChecked(BreakpointRef::CONDITION, row, Qt::Checked);
	}
	const QString cmdStr = Breakpoints::createSetCommand(Breakpoint::CONDITION, {}, {}, {}, condition);

	auto* command = new Command(cmdStr,
		[=] (const QString& id) {
			auto tableIndex = getTableIndexByRow(type, row);
			assert(tableIndex);
			auto* ref = findBreakpointRef(type, row);

			if (ref == nullptr) {
				// ref doesn't exist, so it must be a new breakpoint
				BreakpointRef ref = {type, id, *tableIndex, -1};
				addBreakpointRef(id, ref);
			}
			setBreakpointChecked(type, row, Qt::Checked);

			// update breakpoint id
			setTextField(type, row, BREAKPOINT_ID, id);
			// restore default behaviour if replacing a Breakpoint
			disableRefresh = false;
		},
		[this] (const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

void BreakpointViewer::setBreakpointChecked(BreakpointRef::Type type, int row, Qt::CheckState state)
{
	auto sa = ScopedAssign(userMode, false);

	auto* table    = tables[type];
	bool  oldValue = table->isSortingEnabled();
	auto* item     = table->item(row, ENABLED);
	assert(item);

	table->setSortingEnabled(false);
	item->setCheckState(state);
	table->setSortingEnabled(oldValue);
}

void BreakpointViewer::setTextField(BreakpointRef::Type type, int row, int column, const QString& value, const QString& tooltip)
{
	auto sa = ScopedAssign(userMode, false);

	auto* table    = tables[type];
	bool  oldValue = table->isSortingEnabled();
	auto* item     = table->item(row, column);

	table->setSortingEnabled(false);
	item->setText(value);
	item->setToolTip(tooltip);
	table->setSortingEnabled(oldValue);
}

Slot BreakpointViewer::parseSlotField(std::optional<int> index, const QString& field)
{
	QStringList s = field.simplified().split("/", Qt::SplitBehaviorFlags::SkipEmptyParts);

	std::optional<int8_t> ps;
	if (s.size() > 0 && s[0].compare("X", Qt::CaseInsensitive)) {
		ps = stringToValue<int8_t>(s[0]);
		if (!ps || !(0 <= *ps && *ps <= 3)) {
			if (!s[0].isEmpty()) {
				return {};
			}
			// restore value
			ps = index ? breakpoints->getBreakpoint(*index).slot.ps : std::nullopt;
		}
	}

	std::optional<int8_t> ss;
	if (s.size() == 2 && s[1].compare("X", Qt::CaseInsensitive)) {
		ss = stringToValue<int8_t>(s[1]);
		if (!ss || !(0 <= *ss && *ss <= 3)) {
			if (!s[1].isEmpty()) {
				return {};
			}
			// restore value
			ss = index ? breakpoints->getBreakpoint(*index).slot.ss : std::nullopt;
		}
	}
	return Slot{ps, ss};
}

std::optional<uint8_t> BreakpointViewer::parseSegmentField(std::optional<int> index, const QString& field)
{
	QString s = field.simplified();

	if (s.compare("X", Qt::CaseInsensitive)) {
		auto value = stringToValue<uint8_t>(s);
		return value ? value
		     : (index ? breakpoints->getBreakpoint(*index).segment : std::nullopt);
	}
	return {};
}

std::optional<AddressRange> parseAddressRange(const QString& location)
{
	auto address = stringToValue<uint16_t>(location);
	if (address) return AddressRange{*address};
	return {};
}

std::optional<AddressRange> BreakpointViewer::parseSymbolOrValue(const QString& field) const
{
	if (Symbol* s = debugSession.symbolTable().getAddressSymbol(field)) {
		return AddressRange{uint16_t(s->value())};
	}
	return parseAddressRange(field);
}

QString BreakpointViewer::findSymbolOrValue(uint16_t address) const
{
	if (Symbol* s = debugSession.symbolTable().getAddressSymbol(address)) {
		return s->text();
	}
	return hexValue(address, 4);
}

static const char* ComboTypeNames[] = { "read_mem", "write_mem", "read_io", "write_io" };

std::optional<AddressRange> BreakpointViewer::parseLocationField(
	std::optional<int> bpIndex, BreakpointRef::Type type, const QString& field, const QString& comboTxt)
{
	if (type == BreakpointRef::BREAKPOINT) {
		auto value = parseSymbolOrValue(field);
		return value ? AddressRange{*value}
		    : (bpIndex ? breakpoints->getBreakpoint(*bpIndex).range : std::optional<AddressRange>());
	}

	QStringList s = field.simplified().split(":", Qt::SplitBehaviorFlags::SkipEmptyParts);
	auto begin = s.size() >= 1 ? stringToValue<uint16_t>(s[0]) : std::nullopt;
	auto end = s.size() == 2 ? stringToValue<uint16_t>(s[1]) : std::nullopt;
	auto it = ranges::find(ComboTypeNames, comboTxt);
	auto wType = static_cast<Breakpoint::Type>(std::distance(ComboTypeNames, it) + 1);
	if ((wType == Breakpoint::WATCHPOINT_IOREAD || wType == Breakpoint::WATCHPOINT_IOWRITE)
	     && ((begin && *begin > 0xFF) || (end && *end > 0xFF))) {
		return {};
	}
	if (begin) {
		if (end && *end > *begin) {
			return AddressRange{*begin, end};
		}
		if (!end || *end == *begin) {
			return AddressRange{*begin};
		}
	}

	if (!bpIndex) return {};
	return breakpoints->getBreakpoint(*bpIndex).range;
}

void BreakpointViewer::changeTableItem(BreakpointRef::Type type, QTableWidgetItem* item)
{
	if (!userMode) return;
	auto* table = tables[type];
	int row = table->row(item);

	// trying to modify a bp instead of creating a new one
	bool createBp = false;
	auto ref = findBreakpointRef(type, row);
	auto index = ref ? std::optional<int>(ref->breakpointIndex) : std::nullopt;

	// check if breakpoint is enabled
	auto* enabledItem = table->item(row, ENABLED);
	bool  enabled = enabledItem->checkState() == Qt::Checked;

	switch (table->column(item)) {
		case ENABLED:
			createBp = enabled;
			break;
		case WP_TYPE:
			assert(table == wpTableWidget);
			item = table->item(row, LOCATION);
			[[fallthrough]];
		case LOCATION: {
			auto* model = table->model();
			auto* combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
			int adrLen;

			if (type == BreakpointRef::CONDITION) {
				return;
			} else if (type == BreakpointRef::WATCHPOINT) {
				auto wType = static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
				adrLen = (wType == Breakpoint::WATCHPOINT_IOREAD || wType == Breakpoint::WATCHPOINT_IOWRITE)
				       ? 2 : 4;
			} else {
				adrLen = 4;
			}

			if (auto range = parseLocationField(index, type, item->text(), combo ? combo->currentText() : "")) {
				auto rangeStr = locationString(*range, adrLen);
				// Use a symbolic address in the location field if available
				QString location = debugSession.symbolTable().getAddressSymbol(item->text()) ? item->text() : rangeStr;
				setTextField(type, row, LOCATION, location, location != rangeStr ? rangeStr : "");
			} else {
				enabled = false;
				setTextField(type, row, LOCATION, "");
				setBreakpointChecked(type, row, Qt::Unchecked);
			}
			if (!enabled) return;
			break;
		}
		case SLOT: {
			auto slot = parseSlotField(index, item->text());
			auto [ps, ss] = slot;
			setTextField(type, row, SLOT, QString("%1/%2")
			        .arg(ps ? QChar('0' + *ps) : QChar('X'))
			        .arg(ss ? QChar('0' + *ss) : QChar('X')));
			if (!enabled) return;
			break;
		}
		case SEGMENT: {
			auto segment = parseSegmentField(index, item->text());
			setTextField(type, row, SEGMENT, segment ? QString::number(*segment) : "X");
			if (!enabled) return;
			break;
		}
		case T_CONDITION: {
			setTextField(type, row, T_CONDITION, item->text().simplified());

			if (type == BreakpointRef::CONDITION) {
				setBreakpointChecked(type, row, item->text().isEmpty() ? Qt::Unchecked : item->checkState());
			}
			if (!enabled) return;
			break;
		}
		case BREAKPOINT_ID:
			return;
		default:
			qWarning() << "Unknown table column" << table->column(item);
			assert(false);
	}

	if (enabled) {
		if (createBp) {
			createBreakpoint(type, row);
		} else {
			replaceBreakpoint(type, row);
		}
	} else {
		if (index) removeBreakpoint(type, row, false);
	}
}

void BreakpointViewer::disableSorting(BreakpointRef::Type type)
{
	auto unsort = [](auto* table) {
		table->sortByColumn(-1, Qt::AscendingOrder);
	};

	if (type == BreakpointRef::ALL) {
		for (auto* table : tables) {
			unsort(table);
		}
	} else {
		unsort(tables[type]);
	}
}

void BreakpointViewer::changeBpTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	disableSorting(BreakpointRef::BREAKPOINT);
	changeTableItem(BreakpointRef::BREAKPOINT, item);
}

void BreakpointViewer::changeWpTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	disableSorting(BreakpointRef::WATCHPOINT);
	changeTableItem(BreakpointRef::WATCHPOINT, item);
}

void BreakpointViewer::changeCnTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	disableSorting(BreakpointRef::CONDITION);
	changeTableItem(BreakpointRef::CONDITION, item);
}

void BreakpointViewer::setBreakpoints(Breakpoints* bps)
{
	breakpoints = bps;
}

void BreakpointViewer::setRunState()
{
	runState = true;
}

void BreakpointViewer::setBreakState()
{
	runState = false;
}

void BreakpointViewer::stretchTable(BreakpointRef::Type type)
{
	auto stretch = [](auto* table) {
		// stretching will not work without sorting
		table->setSortingEnabled(true);
		table->resizeColumnsToContents();
	};

	if (type == BreakpointRef::ALL) {
		for (auto* table : tables) {
			stretch(table);
		}
	} else {
		stretch(tables[type]);
	}
}

BreakpointRef* BreakpointViewer::findBreakpointRefById(BreakpointRef::Type type, const QString& id)
{
	auto it = std::find_if(maps[type].begin(), maps[type].end(),
		[&id](std::pair<const QString, BreakpointRef>& tmp) -> bool { return id == tmp.second.id; }
	);
	if (it != maps[type].end()) return &it->second;
	return nullptr;
}

bool BreakpointViewer::addBreakpointRef(const QString& id, BreakpointRef& ref)
{
	auto [_, result] = maps[ref.type].try_emplace(id, ref);
	return result;
}

std::optional<int> BreakpointViewer::findTableRowByIndex(BreakpointRef::Type type, int index) const
{
	auto* table = tables[type];

	for (int row = 0; row < table->rowCount(); ++row) {
		auto* item = table->item(row, TABLE_INDEX);
		int tmp = item->text().toInt();
		if (index == tmp) return row;
	}
	return {};
}

void BreakpointViewer::refresh()
{
	// don't reload if self-inflicted update
	if (disableRefresh) return;

	// store unused items position by disabling ordering
	disableSorting();

	// reset breakpoint index
	for (int type = 0; type < BreakpointRef::ALL; ++type) {
		for (auto& [_, ref] : maps[type]) {
			ref.breakpointIndex = -1;
		}
	}

	for (int bpIndex = 0; bpIndex < breakpoints->breakpointCount(); ++bpIndex) {
		const Breakpoint& bp = breakpoints->getBreakpoint(bpIndex);

		BreakpointRef::Type type = bp.type == Breakpoint::BREAKPOINT ? BreakpointRef::BREAKPOINT
		                         : (bp.type == Breakpoint::CONDITION  ? BreakpointRef::CONDITION : BreakpointRef::WATCHPOINT);

		BreakpointRef* ref = findBreakpointRefById(type, bp.id);
		if (ref != nullptr && ref->breakpointIndex == -1) {
			auto row = findTableRowByIndex(ref->type, ref->tableIndex);
			if (row) {
				// reattach old breakpoints
				auto sa = ScopedAssign(userMode, false);
				ref->breakpointIndex = bpIndex;
				fillTableRow(type, *row, bpIndex);

				// update symbol name
				if (type == BreakpointRef::BREAKPOINT) {
					auto* location = tables[type]->item(*row, LOCATION);
					if (location->text().toUpper() == hexValue(bp.range->start, 4)) {
						fillTableRowLocation(type, *row, findSymbolOrValue(bp.range->start));
					}
				}
			}
		} else {
			// new breakpoints created on OpenMSX will go through here
			auto sa = ScopedAssign(userMode, false);

			auto  row        = createTableRow(type);
			auto  tableIndex = getTableIndexByRow(type, row);

			// create BreakpointRef
			BreakpointRef ref2 = {type, bp.id, *tableIndex, bpIndex};
			addBreakpointRef(bp.id, ref2);
			fillTableRow(type, row, bpIndex);

			// update symbol name
			if (type == BreakpointRef::BREAKPOINT) {
				auto* location = tables[type]->item(row, LOCATION);
				if (location->text().toUpper() == hexValue(bp.range->start, 4)) {
					fillTableRowLocation(type, row, findSymbolOrValue(bp.range->start));
				}
			}
		}
	}

	// remove remaining unpaired BreakpointRef whose breakpoints were replaced or deleted
	for (auto& map : maps) {
		auto it = map.begin();
		while (it != map.end()) {
			auto& ref = it->second;
			auto* table = tables[ref.type];
			it++;

			if (ref.breakpointIndex == -1) {
				auto row = findTableRowByIndex(ref.type, ref.tableIndex);
				if (row) table->removeRow(*row);
				map.erase(ref.id);
			}
		}
	}

	stretchTable(BreakpointRef::ALL);
}

BreakpointRef* BreakpointViewer::findBreakpointRef(BreakpointRef::Type type, int row)
{
	auto& table = tables[type];
	auto* item = table->item(row, BREAKPOINT_ID);
	auto it = maps[type].find(item->text());
	if (it != maps[type].end()) return &it->second;
	return nullptr;
}

void BreakpointViewer::changeCurrentWpType(int row, int /*selected*/)
{
	if (!userMode) return;
	auto* item = wpTableWidget->item(row, WP_TYPE);
	changeTableItem(BreakpointRef::WATCHPOINT, item);
}

void BreakpointViewer::createComboBox(int row)
{
	auto sa = ScopedAssign(userMode, false);

	auto* combo = new QComboBox();
	combo->setEditable(false);
	combo->insertItems(0, QStringList(std::begin(ComboTypeNames), std::end(ComboTypeNames)));

	auto* model = wpTableWidget->model();
	auto  index = model->index(row, WP_TYPE);

	wpTableWidget->setIndexWidget(index, combo);

	connect(combo, qOverload<int>(&QComboBox::currentIndexChanged),
		[this, row](int index){ changeCurrentWpType(row, index); });
}

Breakpoint::Type BreakpointViewer::readComboBox(int row)
{
	auto* model = wpTableWidget->model();
	auto* combo = (QComboBox*) wpTableWidget->indexWidget(model->index(row, WP_TYPE));
	return static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
}

int BreakpointViewer::createTableRow(BreakpointRef::Type type, int row)
{
	auto     sa = ScopedAssign(userMode, false);
	auto& table = tables[type];

	if (row == -1) {
		row = table->rowCount();
		table->setRowCount(row + 1);
		table->selectRow(row);
	} else {
		table->insertRow(row);
		table->selectRow(row);
	}

	// enabled
	auto* item0 = new QTableWidgetItem();
	item0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	item0->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, ENABLED, item0);

	// watchpoint type
	auto* item1 = new QTableWidgetItem();
	item1->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, WP_TYPE, item1);

	if (type == BreakpointRef::WATCHPOINT) {
		createComboBox(row);
	}

	// location
	auto* item2 = new QTableWidgetItem();
	item2->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, LOCATION, item2);

	if (type == BreakpointRef::CONDITION) {
		item2->setFlags(Qt::NoItemFlags);
		item2->setText("0");
	} else {
		item2->setText("");
	}

	// condition
	auto* item3 = new QTableWidgetItem();
	item3->setTextAlignment(Qt::AlignCenter);
	item3->setText("");
	table->setItem(row, T_CONDITION, item3);

	// ps/ss
	auto* item4 = new QTableWidgetItem();
	item4->setTextAlignment(Qt::AlignCenter);
	item4->setText("X/X");
	table->setItem(row, SLOT, item4);

	// segment
	auto* item5 = new QTableWidgetItem();
	item5->setTextAlignment(Qt::AlignCenter);
	item5->setText("X");
	table->setItem(row, SEGMENT, item5);

	// breakpoint id
	auto* item6 = new QTableWidgetItem();
	item6->setFlags(Qt::NoItemFlags);
	item6->setText("");
	table->setItem(row, BREAKPOINT_ID, item6);

	// tableIndex
	auto* item7 = new QTableWidgetItem();
	item7->setFlags(Qt::NoItemFlags);
	item7->setText(QString("%1").arg(newTableItem++));
	table->setItem(row, TABLE_INDEX, item7);

	return row;
}

void BreakpointViewer::fillTableRowLocation(BreakpointRef::Type type, int row, const QString& location)
{
	auto* table = tables[type];
	auto* item = table->item(row, LOCATION);
	item->setText(location);
}

void BreakpointViewer::fillTableRow(BreakpointRef::Type type, int row, int bpIndex)
{
	auto sa = ScopedAssign(userMode, false);

	const auto& bp = breakpoints->getBreakpoint(bpIndex);
	auto& table = tables[type];

	// enabled status
	setBreakpointChecked(type, row, Qt::Checked);

	// watchpoint type
	if (type == BreakpointRef::WATCHPOINT) {
		auto* model = table->model();
		auto* combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
		combo->setCurrentIndex(static_cast<int>(bp.type) - 1);
	}

	// location
	auto* item2 = table->item(row, LOCATION);
	QString location;

	if (bp.type == Breakpoint::BREAKPOINT) {
		location = findSymbolOrValue(bp.range->start);
	}
	if (location.isEmpty()) {
		int adrLen = (bp.type == Breakpoint::WATCHPOINT_IOREAD || bp.type == Breakpoint::WATCHPOINT_IOWRITE) ? 2 : 4;
		location = locationString(*bp.range, adrLen);
	}
	item2->setText(location);

	auto* item3 = table->item(row, T_CONDITION);
	item3->setText(bp.condition);

	// slot
	QString slot = QString("%1/%2")
	    .arg(bp.slot.ps ? QChar('0' + *bp.slot.ps) : QChar('X'))
	    .arg(bp.slot.ss ? QChar('0' + *bp.slot.ss) : QChar('X'));

	auto* item4 = table->item(row, SLOT);
	item4->setText(slot);

	// segment
	QString segment = bp.segment ? QString::number(*bp.segment) : "X";

	auto* item5 = table->item(row, SEGMENT);
	item5->setText(segment);

	// id
	auto* item6 = table->item(row, BREAKPOINT_ID);
	item6->setText(bp.id);
}

std::optional<Breakpoint> BreakpointViewer::parseTableRow(BreakpointRef::Type type, int row)
{
	Breakpoint bp;

	auto* table = tables[type];
	auto* model = table->model();
	auto* combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
	bp.type = type == BreakpointRef::WATCHPOINT ? readComboBox(row) : Breakpoint::BREAKPOINT;

	QString location = table->item(row, LOCATION)->text();
	auto range = parseLocationField({}, type, location, combo ? combo->currentText() : "");
	if (range) bp.range = *range;

	bp.condition = table->item(row, T_CONDITION)->text();
	bp.slot = parseSlotField({}, table->item(row, SLOT)->text());

	auto segment = parseSegmentField({}, table->item(row, SEGMENT)->text());
	if (segment) bp.segment = *segment;

	return bp;
}


void BreakpointViewer::onSymbolTableChanged()
{
	for (int row = 0; row < bpTableWidget->rowCount(); ++row) {
		auto*  ref = findBreakpointRef(BreakpointRef::BREAKPOINT, row);
		auto* item = bpTableWidget->item(row, LOCATION);

		if (ref->breakpointIndex > -1) {
			Breakpoint  bp = breakpoints->getBreakpoint(ref->breakpointIndex);

			if (!item->text().isEmpty()) {
				Symbol* symbol = debugSession.symbolTable().getAddressSymbol(item->text());
				if (symbol) {
					setTextField(BreakpointRef::BREAKPOINT, row, LOCATION, symbol->text());
				} else {
					setTextField(BreakpointRef::BREAKPOINT, row, LOCATION, findSymbolOrValue(bp.range->start));
				}
			}
		}
	}
	stretchTable(BreakpointRef::ALL);
}


void BreakpointViewer::onAddBtnClicked(BreakpointRef::Type type)
{
	auto sa = ScopedAssign(userMode, false);

	auto* table = tables[type];
	table->sortByColumn(-1, Qt::AscendingOrder);
	table->setSortingEnabled(false);

	int row = createTableRow(type, 0);
	setBreakpointChecked(type, row, Qt::Unchecked);
	stretchTable(type);
	table->setSortingEnabled(true);
}

void BreakpointViewer::onRemoveBtnClicked(BreakpointRef::Type type)
{
	auto* table = tables[type];

	if (table->currentRow() == -1)
		return;

	auto sa = ScopedAssign(userMode, false);
	QTableWidgetItem* item = table->item(table->currentRow(), ENABLED);
	if (item->checkState() == Qt::Checked) {
		removeBreakpoint(type, table->currentRow(), true);
	} else {
		table->removeRow(table->currentRow());
	}
}

void BreakpointViewer::on_btnAddBp_clicked()
{
	onAddBtnClicked(BreakpointRef::BREAKPOINT);
}

void BreakpointViewer::on_btnRemoveBp_clicked()
{
	onRemoveBtnClicked(BreakpointRef::BREAKPOINT);
}

void BreakpointViewer::on_btnAddWp_clicked()
{
	onAddBtnClicked(BreakpointRef::WATCHPOINT);
}

void BreakpointViewer::on_btnRemoveWp_clicked()
{
	onRemoveBtnClicked(BreakpointRef::WATCHPOINT);
}

void BreakpointViewer::on_btnAddCn_clicked()
{
	onAddBtnClicked(BreakpointRef::CONDITION);
}

void BreakpointViewer::on_btnRemoveCn_clicked()
{
	onRemoveBtnClicked(BreakpointRef::CONDITION);
}

void BreakpointViewer::on_itemPressed(QTableWidgetItem* /*item*/)
{
	bpTableWidget->setSortingEnabled(false);
}

void BreakpointViewer::on_headerClicked(int /*index*/)
{
	bpTableWidget->setSortingEnabled(true);
}
