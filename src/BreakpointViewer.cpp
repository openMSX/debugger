#include "BreakpointViewer.h"
#include "Convert.h"
#include "CommClient.h"
#include "OpenMSXConnection.h"
#include "ScopedAssign.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QComboBox>
#include <cassert>

enum TableColumns {
	ENABLED = 0,
	WP_TYPE = 1,
	LOCATION = 2,
	BP_ADDRESS = 2,
	WP_REGION = 2,
	T_CONDITION = 3,
	SLOT = 4,
	SEGMENT = 5,
	ID = 6
};

BreakpointViewer::BreakpointViewer(QWidget* parent)
	: QTabWidget(parent),
	  ui(new Ui::BreakpointViewer)
{
	setupUi(this);

	bpTableWidget->sortByColumn(BP_ADDRESS, Qt::AscendingOrder);
	bpTableWidget->setColumnHidden(WP_TYPE, true);
	bpTableWidget->setColumnHidden(ID, true);
	bpTableWidget->resizeColumnsToContents();
	bpTableWidget->setSortingEnabled(true);
	connect(bpTableWidget, &QTableWidget::itemPressed, this, &BreakpointViewer::on_itemPressed);
	connect(bpTableWidget, &QTableWidget::itemChanged, this, &BreakpointViewer::changeBpTableItem);
	connect(bpTableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this,
	        &BreakpointViewer::on_headerClicked);

	wpTableWidget->setColumnHidden(ID, true);
	wpTableWidget->sortByColumn(WP_REGION, Qt::AscendingOrder);
	wpTableWidget->resizeColumnsToContents();
	wpTableWidget->setSortingEnabled(true);
	connect(wpTableWidget, &QTableWidget::itemChanged, this,
	        &BreakpointViewer::changeWpTableItem);

	cnTableWidget->setColumnHidden(WP_TYPE, true);
	cnTableWidget->setColumnHidden(LOCATION, true);
	cnTableWidget->setColumnHidden(SLOT, true);
	cnTableWidget->setColumnHidden(SEGMENT, true);
	cnTableWidget->setColumnHidden(ID, true);
	cnTableWidget->sortByColumn(T_CONDITION, Qt::AscendingOrder);
	cnTableWidget->resizeColumnsToContents();
	cnTableWidget->setSortingEnabled(true);
	connect(cnTableWidget, &QTableWidget::itemChanged, this,
	        &BreakpointViewer::changeCnTableItem);

	tables[BreakpointData::BREAKPOINT] = bpTableWidget;
	tables[BreakpointData::WATCHPOINT] = wpTableWidget;
	tables[BreakpointData::CONDITION]  = cnTableWidget;
}

// TODO: move the createSetCommand to a session manager
void BreakpointViewer::createBreakpoint(BreakpointData::Type type, int row)
{
	if (type == BreakpointData::CONDITION) _createCondition(row);
	else _createBreakpoint(type, row);
}

void BreakpointViewer::_handleSyncError(const QString& error)
{
	int choice = QMessageBox::warning(nullptr, tr("Synchronization error"),
			tr("Error message from openMSX:\n\"%1\"\n\nClick on Reset to resynchronize with openMSX")
			.arg(error.trimmed()), QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		auto sa = ScopedAssign(selfUpdating, false);
		emit contentsUpdated(false);
	}
}

void BreakpointViewer::_handleKeyAlreadyExists()
{
	int choice = QMessageBox::warning(nullptr, tr("Reference error"),
			tr("A breakpoint with the same name already exists locally. This is probably "
			   "a synchronization problem.\n\nClick on Reset to resynchronize with openMSX"),
			QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		auto sa = ScopedAssign(selfUpdating, false);
		emit contentsUpdated(false);
	}
}

void BreakpointViewer::_handleKeyNotFound()
{
	int choice = QMessageBox::warning(nullptr, tr("Reference error"),
			tr("Breakpoint was not found locally. This is probably a synchronization problem.\n\n"
			   "Click on Reset to resynchronize with openMSX"),
			QMessageBox::Reset | QMessageBox::Cancel);
	if (choice == QMessageBox::Reset) {
		auto sa = ScopedAssign(selfUpdating, false);
		emit contentsUpdated(false);
	}
}

void BreakpointViewer::_createBreakpoint(BreakpointData::Type type, int row)
{
	assert(type != BreakpointData::CONDITION);
	auto* table            = tables[type];
	auto* model            = table->model();
	auto* combo            = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
	Breakpoint::Type wtype = type == BreakpointData::WATCHPOINT ? readComboBox(row) : Breakpoint::BREAKPOINT;

	int address, endRange;
	QString location = table->item(row, LOCATION)->text();
	bool ok1 = parseLocationField(-1, type, location, address, endRange, combo ? combo->currentText() : "");
	if (!ok1) {
		setBreakpointChecked(type, row, Qt::Unchecked);
		return;
	} else {
		setBreakpointChecked(type, row, Qt::Checked);
	}

	QString condition = table->item(row, T_CONDITION)->text();

	qint8 ps, ss;
	bool ok2 = parseSlotField(-1, table->item(row, SLOT)->text(), ps, ss);
	assert(ok2);

	qint16 segment;
	bool ok3 = parseSegmentField(-1, table->item(row, SEGMENT)->text(), segment);
	assert(ok3);

	const QString cmdStr = Breakpoints::createSetCommand(wtype, address, ps, ss, segment, endRange, condition);

	auto* command = new Command(cmdStr,
		[this, type, row] (const QString& id) {
			setTextField(type, row, ID, id);

			BreakpointData data { type, id, row, -1 };
			if (!connectBreakpointID(id, data)) {
				setBreakpointChecked(type, row, Qt::Unchecked);
				_handleKeyAlreadyExists();
				return;
			}

			auto sa = ScopedAssign(selfUpdating, true);
			emit contentsUpdated(false);
		},
		[this] (const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::replaceBreakpoint(BreakpointData::Type type, int row)
{
	auto* table = tables[type];
	auto* item  = table->item(row, ID);
	QString id  = item->text();

	const QString cmdStr = breakpoints->createRemoveCommand(id);
	auto* command = new Command(cmdStr,
		[this, type, row] (const QString& /*result*/) {
			createBreakpoint(type, row);
		},
		[this](const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::removeBreakpoint(BreakpointData::Type type, int row, bool logical)
{
	auto* table = tables[type];

	auto* item  = table->item(row, ID);
	assert(!item->text().isEmpty());
	QString id  = item->text();

	const QString cmdStr = Breakpoints::createRemoveCommand(id);

	auto* command = new Command(cmdStr,
		[this, type, row, id, logical] (const QString& /*result*/) {
			size_t erased = maps[type].erase(id);

			if (erased != 1) {
				setBreakpointChecked(type, row, Qt::Unchecked);
				_handleKeyNotFound();
				return;
			}

			if (!logical) {
				auto* table = tables[type];
				auto     sa = ScopedAssign(userMode, false);
				table->removeRow(row);
			}

			auto sa = ScopedAssign(selfUpdating, true);
			emit contentsUpdated(false);
		},
		[this] (const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

// TODO: move the createSetCommand to a session manager
void BreakpointViewer::_createCondition(int row)
{
	QString condition = cnTableWidget->item(row, T_CONDITION)->text();
	if (condition.isEmpty()) {
		setBreakpointChecked(BreakpointData::CONDITION, row, Qt::Unchecked);
		return;
	} else {
		setBreakpointChecked(BreakpointData::CONDITION, row, Qt::Checked);
	}

	const QString cmdStr = Breakpoints::createSetCommand(Breakpoint::CONDITION, -1, -1, -1, -1, -1, condition);

	auto* command = new Command(cmdStr,
		[this, row] (const QString& id) {
			setTextField(BreakpointData::CONDITION, row, ID, id);

			BreakpointData data { BreakpointData::CONDITION, id, row, -1 };
			if (!connectBreakpointID(id, data)) {
				_handleKeyAlreadyExists();
				return;
			}

			auto sa = ScopedAssign(selfUpdating, true);
			emit contentsUpdated(false);
		},
		[this] (const QString& error) { _handleSyncError(error); }
	);

	CommClient::instance().sendCommand(command);
}

void BreakpointViewer::setBreakpointChecked(BreakpointData::Type type, int row, Qt::CheckState state)
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

void BreakpointViewer::setTextField(BreakpointData::Type type, int row, int column, const QString& value)
{
	auto sa = ScopedAssign(userMode, false);

	auto* table    = tables[type];
	bool  oldValue = table->isSortingEnabled();
	auto* item     = table->item(row, column);

	table->setSortingEnabled(false);
	item->setText(value);
	table->setSortingEnabled(oldValue);
}

bool BreakpointViewer::parseSlotField(int index, const QString& field, qint8& ps, qint8& ss)
{
	QStringList s = field.simplified().split("/", Qt::SplitBehaviorFlags::SkipEmptyParts);

	if (s[0].compare("X", Qt::CaseInsensitive)) {
		bool ok;
		int value = s[0].toInt(&ok);
		if (!ok || (value < 0 && value > 3)) {
			// restore value
			if (index != -1) {
				ps = breakpoints->getBreakpoint(index).ps;
			} else {
				return false;
			}
		} else {
			ps = value;
		}
	} else {
		ps = -1;
	}
	if (s.size() == 2 && s[1].compare("X", Qt::CaseInsensitive)) {
		bool ok;
		int value = s[1].toInt(&ok);
		if (!ok || (value < 0 && value > 3)) {
			if (index != -1) {
				ss = breakpoints->getBreakpoint(index).ss;
			} else {
				return false;
			}
		} else {
			ss = value;
		}
	} else {
		ss = -1;
	}
	return true;
}

bool BreakpointViewer::parseSegmentField(int index, const QString& field, qint16& segment)
{
	QString s = field.simplified();

	if (s.compare("X", Qt::CaseInsensitive)) {
		bool ok;
		int value = field.toInt(&ok);
		if (!ok || (value < 0 && value > 255)) {
			if (index != -1) {
				segment = breakpoints->getBreakpoint(index).segment;
			} else {
				return false;
			}
		} else {
			segment = value;
		}
	} else {
		segment = -1;
	}
	return true;
}

static const char* ComboTypeNames[] = { "read_mem", "write_mem", "read_io", "write_io" };

bool BreakpointViewer::parseLocationField(int index, BreakpointData::Type type,
		const QString& field, int& begin, int& end, const QString& comboTxt)
{
	if (type == BreakpointData::BREAKPOINT) {
		if (int value = stringToValue(field.simplified()); value > -1 && value < 0x10000) {
			begin = value;
			end = -1;
			return true;
		} else if (index != -1) {
			begin = breakpoints->getBreakpoint(index).address;
			end = -1;
			return true;
		}
		return false;
	}

	QStringList s = field.simplified().split(":", Qt::SplitBehaviorFlags::SkipEmptyParts);
	begin = (s.size() >= 1) ? stringToValue(s[0]) : -1;
	end   = (s.size() == 2) ? stringToValue(s[1]) : (s.size() == 1 ? begin : -1);

	auto iter  = std::find(std::begin(ComboTypeNames), std::end(ComboTypeNames), comboTxt);
	auto wtype = static_cast<Breakpoint::Type>(std::distance(ComboTypeNames, iter) + 1);
	if ((wtype == Breakpoint::WATCHPOINT_IOREAD || wtype == Breakpoint::WATCHPOINT_IOWRITE)
	     && (begin > 0xFF || end > 0xFF)) {
		return false;
	}
	if (begin != -1 && end != -1 && end >= begin) return true;

	if (index == -1) return false;
	const auto& bp = breakpoints->getBreakpoint(index);
	begin    = bp.address;
	end      = bp.regionEnd;
	return true;
}

void BreakpointViewer::changeTableItem(BreakpointData::Type type, QTableWidgetItem* item)
{
	auto* table = tables[type];
	int   row   = table->row(item);

	// trying to modify a bp instead of creating a new one
	bool  createBp = false;
	auto* idItem   = table->item(row, ID);
	auto  it       = findBreakpointData(type, idItem->text());
	int   index    = it != maps[type].end() ? std::distance(maps[type].begin(), it) : -1;

	// check if breakpoint is enabled
	auto* enabledItem = table->item(row, ENABLED);
	bool  enabled     = enabledItem->checkState() == Qt::Checked;

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
			int   adrLen;

			if (type == BreakpointData::CONDITION) {
				return;
			} else if (type == BreakpointData::WATCHPOINT) {
				auto wType = static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
				adrLen = (wType == Breakpoint::WATCHPOINT_IOREAD || wType == Breakpoint::WATCHPOINT_IOWRITE)
					? 2 : 4;
			} else {
				adrLen = 4;
			}

			int begin, end;
			if (parseLocationField(index, type, item->text(), begin, end, combo ? combo->currentText() : "")) {
				setTextField(type, row, LOCATION, QString("%1%2%3").arg(hexValue(begin, adrLen))
					.arg(end == begin || end == -1 ? "" : ":")
					.arg(end == begin || end == -1 ? "" : hexValue(end, adrLen)));
			} else {
				enabled = false;
				setTextField(type, row, LOCATION, "");
				setBreakpointChecked(type, row, Qt::Unchecked);
			}
			if (!enabled) return;
			break;
		}
		case SLOT: {
			qint8 ps, ss;
			if (parseSlotField(index, item->text(), ps, ss)) {
				setTextField(type, row, SLOT, QString("%1/%2")
					.arg(ps == -1 ? QChar('X') : QChar('0' + ps))
					.arg(ss == -1 ? QChar('X') : QChar('0' + ss)));
			} else {
				setTextField(type, row, SLOT, "X/X");
			}
			if (!enabled) return;
			break;
		}
		case SEGMENT: {
			qint16 segment;
			if (parseSegmentField(index, item->text(), segment)) {
				setTextField(type, row, SEGMENT, QString("%1").arg(segment == -1 ? "X" : QString::number(segment)));
			} else {
				setTextField(type, row, SEGMENT, "X");
			}
			if (!enabled) return;
			break;
		}
		case T_CONDITION: {
			setTextField(type, row, T_CONDITION, item->text().simplified());

			if (type == BreakpointData::CONDITION) {
				setBreakpointChecked(type, row, item->text().isEmpty() ? Qt::Unchecked : item->checkState());
			}
			if (!enabled) return;
			break;
		}
		case ID:
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
		if (index != -1) removeBreakpoint(type, row, true);
	}
}

void BreakpointViewer::disableSorting(BreakpointData::Type type)
{
	auto unsort = [](auto* table) {
		table->sortByColumn(-1, Qt::AscendingOrder);
	};

	if (type == BreakpointData::ALL) {
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
	disableSorting(BreakpointData::BREAKPOINT);
	changeTableItem(BreakpointData::BREAKPOINT, item);
}

void BreakpointViewer::changeWpTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	disableSorting(BreakpointData::WATCHPOINT);
	changeTableItem(BreakpointData::WATCHPOINT, item);
}

void BreakpointViewer::changeCnTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	disableSorting(BreakpointData::CONDITION);
	changeTableItem(BreakpointData::CONDITION, item);
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

void BreakpointViewer::stretchTable(BreakpointData::Type type)
{
	auto stretch = [](auto* table) {
		// stretching will not work without sorting
		table->setSortingEnabled(true);
		table->resizeColumnsToContents();
	};

	if (type == BreakpointData::ALL) {
		for (auto* table : tables) {
			stretch(table);
		}
	} else {
		stretch(tables[type]);
	}
}

std::map<QString, BreakpointData>::iterator
BreakpointViewer::scanBreakpointData(const Breakpoint& bp)
{
	BreakpointData::Type type = bp.type == Breakpoint::BREAKPOINT ? BreakpointData::BREAKPOINT
	                         : (bp.type == Breakpoint::CONDITION  ? BreakpointData::CONDITION : BreakpointData::WATCHPOINT);
	if (maps[type].empty()) return maps[type].end();

	// assumed premise: a new bp will never repeat same name again
	if (auto it = maps[type].find(bp.id); it != maps[type].end() && it->second.index == -1) {
		// assumed premise 2: if name remains the same, bp is exactly the same.
		return it;
	}

	return maps[type].end();
}

bool BreakpointViewer::connectBreakpointID(const QString& id, BreakpointData& data)
{
	auto [_, result] = maps[data.type].try_emplace(id, data);
	return result;
}

void BreakpointViewer::sync()
{
	// don't reload if self-inflicted update
	if (selfUpdating) return;

	bool empty[BreakpointData::ALL] = {false, false, false};

	for (int type = 0; type < BreakpointData::ALL; ++type) {
		if (maps[type].empty()) empty[type] = true;

		for (auto& [_, data] : maps[type]) {
			data.index = -1;
			data.row = -1;
		}
	}

	for (int index = 0; index < breakpoints->breakpointCount(); ++index) {
		const Breakpoint& bp = breakpoints->getBreakpoint(index);

		BreakpointData::Type type = bp.type == Breakpoint::BREAKPOINT ? BreakpointData::BREAKPOINT
		                         : (bp.type == Breakpoint::CONDITION  ? BreakpointData::CONDITION : BreakpointData::WATCHPOINT);

		if (!empty[type]) {
			if (auto it = scanBreakpointData(bp); it != maps[type].end()) {
				it->second.index = index;
				continue;
			}
		}

		// create new BreakpointDatas
		BreakpointData data { type, bp.id, -1, index };
		bool result = connectBreakpointID(bp.id, data);
		assert(result);
	}

	// remove missing BreakpointData
	for (auto& map : maps) {
		auto it = map.begin();

		while (it != map.end()) {
			if (it->second.index == -1) {
				auto oldit = it++;
				map.erase(oldit->second.id);
			} else {
				it++;
			}
		}
	}

	populate();
}

void BreakpointViewer::populate()
{
	// store unused items position by disabling ordering
	disableSorting();

	// remember selected rows
	int bpFocus = bpTableWidget->currentRow();
	int wpFocus = wpTableWidget->currentRow();
	int cnFocus = cnTableWidget->currentRow();

	// reuse current table rows
	for (int index = 0; index < BreakpointData::ALL; ++index) {
		auto type = static_cast<BreakpointData::Type>(index);
		auto* table = tables[index];

		for (int row = 0; row < table->rowCount(); ++row) {
			auto* idItem = table->item(row, ID);
			assert(idItem);
			auto  it     = findBreakpointData(type, idItem->text());

			// update reused breakpoint
			if (it != maps[type].end()) {
				it->second.row = row;
			} else {
				auto* enabledItem = table->item(row, ENABLED);
				enabledItem->setCheckState(Qt::Unchecked);
			}
		}
	}

	// add only new breakpoints
	for (auto& map : maps) {
		for (auto& [_, data] : map) {
			if (data.row != -1) continue;
			data.row = createTableRow(data.type);
			fillTableRow(data.index, data.type, data.row);
		}
	}

	stretchTable(BreakpointData::ALL);

	bpTableWidget->selectRow(bpFocus);
	wpTableWidget->selectRow(wpFocus);
	cnTableWidget->selectRow(cnFocus);
}

std::map<QString, BreakpointData>::iterator
BreakpointViewer::findBreakpointData(BreakpointData::Type type, const QString& id)
{
	return maps[type].find(id);
}

void BreakpointViewer::changeCurrentWpType(int row, int /*selected*/)
{
	if (!userMode) return;
	auto* item = wpTableWidget->item(row, WP_TYPE);
	changeTableItem(BreakpointData::WATCHPOINT, item);
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

	connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		[this, row](int index){ changeCurrentWpType(row, index); });
}

Breakpoint::Type BreakpointViewer::readComboBox(int row)
{
	auto* model = wpTableWidget->model();
	auto* combo = (QComboBox*) wpTableWidget->indexWidget(model->index(row, WP_TYPE));
	return static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
}

int BreakpointViewer::createTableRow(BreakpointData::Type type, int row)
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

	if (type == BreakpointData::WATCHPOINT) {
		createComboBox(row);
	}

	// address
	auto* item2 = new QTableWidgetItem();
	item2->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, LOCATION, item2);

	if (type == BreakpointData::CONDITION) {
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

	// index
	auto* item6 = new QTableWidgetItem();
	item6->setFlags(Qt::NoItemFlags);
	item6->setText("");
	table->setItem(row, ID, item6);

	return row;
}

void BreakpointViewer::fillTableRow(int index, BreakpointData::Type type, int row)
{
	auto sa = ScopedAssign(userMode, false);

	const auto& bp = breakpoints->getBreakpoint(index);
	auto& table = tables[type];

	// enabled status
	setBreakpointChecked(type, row, Qt::Checked);

	// watchpoint type
	if (type == BreakpointData::WATCHPOINT) {
	        auto* model = table->model();
		auto* combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
		combo->setCurrentIndex(static_cast<int>(bp.type) - 1);
	}

	// location
	int locLen = (bp.type == Breakpoint::WATCHPOINT_IOREAD
	           || bp.type == Breakpoint::WATCHPOINT_IOWRITE) ? 2 : 4;
	QString address   = hexValue(bp.address, locLen);
	QString regionEnd = hexValue(bp.regionEnd, locLen);
	QString location  = QString("%1%2%3").arg(address)
		.arg(regionEnd == -1 || regionEnd == address ? "" : ":")
		.arg(regionEnd == -1 || regionEnd == address ? "" : regionEnd);

	auto* item2 = table->item(row, LOCATION);
	item2->setText(location);

	auto* item3 = table->item(row, T_CONDITION);
	item3->setText(bp.condition);

	// slot
	QString slot = QString("%1/%2")
		.arg(bp.ps == -1 ? QChar('X') : QChar('0' + bp.ps))
		.arg(bp.ss == -1 ? QChar('X') : QChar('0' + bp.ss));

	auto* item4 = table->item(row, SLOT);
	item4->setText(slot);

	// segment
	QString segment = (bp.segment == -1 ? "X" : QString("%1").arg(bp.segment));

	auto* item5 = table->item(row, SEGMENT);
	item5->setText(segment);

	auto* item6 = table->item(row, ID);
	item6->setText(bp.id);
}

void BreakpointViewer::onAddBtnClicked(BreakpointData::Type type)
{
	auto     sa = ScopedAssign(userMode, false);

	auto* table = tables[type];
	table->sortByColumn(-1, Qt::AscendingOrder);
	table->setSortingEnabled(false);

	int row = createTableRow(type, 0);
	setBreakpointChecked(type, row, Qt::Unchecked);
	stretchTable(type);
	table->setSortingEnabled(true);
}

void BreakpointViewer::onRemoveBtnClicked(BreakpointData::Type type)
{
	auto* table = tables[type];

	if (table->currentRow() == -1)
		return;

	auto sa = ScopedAssign(userMode, false);
	QTableWidgetItem* item = table->item(table->currentRow(), ENABLED);
	if (item->checkState() == Qt::Checked) {
		removeBreakpoint(type, table->currentRow(), false);
	} else {
		table->removeRow(table->currentRow());
	}
}

void BreakpointViewer::on_btnAddBp_clicked()
{
	onAddBtnClicked(BreakpointData::BREAKPOINT);
}

void BreakpointViewer::on_btnRemoveBp_clicked()
{
	onRemoveBtnClicked(BreakpointData::BREAKPOINT);
}

void BreakpointViewer::on_btnAddWp_clicked()
{
	onAddBtnClicked(BreakpointData::WATCHPOINT);
}

void BreakpointViewer::on_btnRemoveWp_clicked()
{
	onRemoveBtnClicked(BreakpointData::WATCHPOINT);
}

void BreakpointViewer::on_btnAddCn_clicked()
{
	onAddBtnClicked(BreakpointData::CONDITION);
}

void BreakpointViewer::on_btnRemoveCn_clicked()
{
	onRemoveBtnClicked(BreakpointData::CONDITION);
}

void BreakpointViewer::on_itemPressed(QTableWidgetItem* /*item*/)
{
	bpTableWidget->setSortingEnabled(false);
}

void BreakpointViewer::on_headerClicked(int /*index*/)
{
	bpTableWidget->setSortingEnabled(true);
}
