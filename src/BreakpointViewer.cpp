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
	CONDITION = 3,
	SLOT = 4,
	SEGMENT = 5,
};

bool BreakpointViewer::ItemPos::operator==(const ItemPos& ip) const
{
	return (ip.type == type) && (ip.row == row);
}

BreakpointViewer::BreakpointViewer(QWidget* parent)
	: QTabWidget(parent),
	  ui(new Ui::BreakpointViewer)
{
	setupUi(this);
	bpTableWidget->sortByColumn(BP_ADDRESS, Qt::AscendingOrder);
	wpTableWidget->sortByColumn(WP_REGION, Qt::AscendingOrder);
	bpTableWidget->setColumnHidden(WP_TYPE, true);
	bpTableWidget->resizeColumnsToContents();
	wpTableWidget->resizeColumnsToContents();

	selector[0] = bpTableWidget;
	selector[1] = wpTableWidget;
	userMode = true;

	connect(bpTableWidget, &QTableWidget::itemChanged, this,
			&BreakpointViewer::changeBpTableItem);
	connect(wpTableWidget, &QTableWidget::itemChanged, this,
			&BreakpointViewer::changeWpTableItem);
}

// TODO: move the createSetCommand to a session manager
void BreakpointViewer::createBreakpoint(Type type, int row)
{
	auto table = selector[type];
	Breakpoint::Type type2;

	if (type == WATCHPOINT) {
		auto model = table->model();
		auto combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
		type2 = static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
	} else {
		type2 = Breakpoint::BREAKPOINT;
	}

	int address, endRange;
	QString location = table->item(row, LOCATION)->text();
	bool ok1 = processLocationField(-1, type, location, address, endRange);
	if (!ok1) {
		auto sa = ScopedAssign(userMode, false);
		setBreakpointStatus(type, row, false);
		return;
	}

	QString condition = table->item(row, CONDITION)->text();

	qint8 ps, ss;
	bool ok2 = processSlotField(-1, table->item(row, SLOT)->text(), ps, ss);
	assert(ok2);

	qint16 segment;
	bool ok3 = processSegmentField(-1, table->item(row, SEGMENT)->text(), segment);
	assert(ok3);

	const QString cmdStr = Breakpoints::createSetCommand(type2, address, ps, ss, segment,
			endRange, condition);
	auto command = new Command(cmdStr,
		[this] (const QString& result) {
			emit contentsUpdated(false);
		}
	);
	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::replaceBreakpoint(Type type, int row)
{
	int index = items.indexOf({type, row});
	assert(index != -1);

	const QString& id = breakpoints->getBreakpoint(index).id;
	const QString  cmdStr = breakpoints->createRemoveCommand(id);

	auto command = new Command(cmdStr,
		[this, type, row] (const QString& result) {
			createBreakpoint(type, row);
		}
	);
	CommClient::instance().sendCommand(command);
}

// TODO: move the createRemoveCommand to a session manager
void BreakpointViewer::removeBreakpoint(Type type, int row)
{
	int index = items.indexOf({type, row});

	const QString& id     = breakpoints->getBreakpoint(index).id;
	const QString  cmdStr = Breakpoints::createRemoveCommand(id);

	auto command = new Command(cmdStr,
		[this] (const QString& result) {
			emit contentsUpdated(false);
	});
	CommClient::instance().sendCommand(command);
}

void BreakpointViewer::setBreakpointStatus(Type type, int row, int status)
{
	auto table = selector[type];
	auto item = table->item(row, ENABLED);
	item->setFlags(status
		? item->flags() |  Qt::ItemIsEnabled
		: item->flags() & ~Qt::ItemIsEnabled);
}

bool BreakpointViewer::processSlotField(int index, const QString& field, qint8& ps, qint8& ss)
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

bool BreakpointViewer::processSegmentField(int index, const QString& field, qint16& segment)
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

bool BreakpointViewer::processLocationField(int index, BreakpointViewer::Type type,
		const QString& field, int& begin, int& end)
{
	if (type == BreakpointViewer::BREAKPOINT) {
		if (int value = stringToValue(field.simplified()); value != -1) {
			begin = 0xFFFF & value;
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
	if (begin != -1 && end != -1 && end >= begin) return true;

	if (index == -1) return false;
	auto bp = breakpoints->getBreakpoint(index);
	begin = bp.address;
	end = bp.regionEnd;
	return true;
}

void BreakpointViewer::changeTableItem(Type type, QTableWidgetItem* item)
{
	auto table = selector[type];

	// trying to modify a bp instead of creating a new one
	bool createBp = false;

	// check if breakpoint is enabled
	int row = table->row(item);
	int index = items.indexOf({type, row});
	QTableWidgetItem* enabledItem = table->item(row, ENABLED);
	bool enabled = enabledItem->checkState() == Qt::Checked;

	switch (table->column(item)) {
		case ENABLED:
			createBp = enabled;
			break;
		case WP_TYPE:
			assert(table == wpTableWidget);
			if (!enabled) return;
			break;
		case LOCATION: {
			int adrLen;

			if (type == WATCHPOINT) {
				auto model = table->model();
				auto combo = (QComboBox*) table->indexWidget(model->index(row, WP_TYPE));
				Breakpoint::Type wtype = static_cast<Breakpoint::Type>(combo->currentIndex() + 1);
				adrLen = (wtype == Breakpoint::WATCHPOINT_IOREAD || wtype == Breakpoint::WATCHPOINT_IOWRITE)
					? 2 : 4;
			} else {
				adrLen = 4;
			}
			int begin, end;

			if (processLocationField(index, type, item->text(), begin, end)) {
				auto sa = ScopedAssign(userMode, false);
				item->setText(QString("%1%2%3").arg(hexValue(begin, adrLen))
					.arg(end == begin || end == -1 ? "" : ":")
					.arg(end == begin || end == -1 ? "" : hexValue(end, adrLen)));
				setBreakpointStatus(type, row, true);
			} else {
				auto sa = ScopedAssign(userMode, false);
				enabled = false;
				item->setText("");
				setBreakpointStatus(type, row, false);
			}
			if (!enabled) return;
			break;
		}
		case SLOT: {
			qint8 ps, ss;
			if (processSlotField(index, item->text(), ps, ss)) {
				auto sa = ScopedAssign(userMode, false);
				item->setText(QString("%1/%2")
					.arg(ps == -1 ? QChar('X') : QChar('0' + ps))
					.arg(ss == -1 ? QChar('X') : QChar('0' + ss)));
			} else {
				auto sa = ScopedAssign(userMode, false);
				item->setText("X/X");
			}
			if (!enabled) return;
			break;
		}
		case SEGMENT: {
			qint16 segment;
			if (processSegmentField(index, item->text(), segment)) {
				auto sa = ScopedAssign(userMode, false);
				item->setText(QString("%1").arg(segment == -1 ? "X" : QString::number(segment)));
			} else {
				auto sa = ScopedAssign(userMode, false);
				item->setText("X");
			}
			if (!enabled) return;
			break;
		}
		case CONDITION: {
			auto sa = ScopedAssign(userMode, false);
			item->setText(item->text().simplified());
			if (!enabled) return;
			break;
		}
	}

	if (enabled) {
		if (createBp) {
			createBreakpoint(type, row);
		} else {
			replaceBreakpoint(type, row);
		}
	} else if (index != -1) {
		removeBreakpoint(type, row);
	} else {
		assert(index != -1);
	}
}

void BreakpointViewer::changeBpTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	changeTableItem(BREAKPOINT, item);
}

void BreakpointViewer::changeWpTableItem(QTableWidgetItem* item)
{
	if (!userMode) return;
	changeTableItem(WATCHPOINT, item);
}

void BreakpointViewer::setBreakpoints(Breakpoints* bps)
{
	breakpoints = bps;
}

void BreakpointViewer::keepUnusedItems()
{
	int row = 0;

	while (row < bpTableWidget->rowCount()) {
		QTableWidgetItem* enabledItem = bpTableWidget->item(row, ENABLED);
		bool enabled = enabledItem->checkState() == Qt::Checked;
		if (enabled) {
			bpTableWidget->removeRow(row);
		} else {
			row++;
		}
	}

	while (row < wpTableWidget->rowCount()) {
		QTableWidgetItem* enabledItem = wpTableWidget->item(row, ENABLED);
		bool enabled = enabledItem->checkState() == Qt::Checked;
		if (enabled) {
			wpTableWidget->removeRow(row);
		} else {
			row++;
		}
	}
}

void BreakpointViewer::setRunState()
{
	runState = true;
}

void BreakpointViewer::setBreakState()
{
	runState = false;
}

void BreakpointViewer::sync()
{
	items.clear();

	auto sa = ScopedAssign(userMode, false);
	keepUnusedItems();

	int bpRow = bpTableWidget->rowCount();
	int wpRow = wpTableWidget->rowCount();

	if (breakpoints->breakpointCount() != -1) {
		for (int index = 0; index < breakpoints->breakpointCount(); ++index) {
			const Breakpoint& bp = breakpoints->getBreakpoint(index);

			switch (bp.type) {
			case Breakpoint::BREAKPOINT: {
				bpRow = createTableRow(BREAKPOINT);
				setBreakpointStatus(BREAKPOINT, bpRow, true);

				bool currentSlot = breakpoints->inCurrentSlot(bp);
				QBrush color = palette().brush(runState | currentSlot ?
					QPalette::ColorGroup::Normal : QPalette::ColorGroup::Disabled,
					QPalette::WindowText);

				QTableWidgetItem* item1 = bpTableWidget->item(bpRow, ENABLED);
				item1->setCheckState(Qt::Checked);

				QTableWidgetItem* item2 = bpTableWidget->item(bpRow, BP_ADDRESS);
				item2->setForeground(color);
				item2->setText(hexValue(bp.address, 4));

				QTableWidgetItem* item3 = bpTableWidget->item(bpRow, CONDITION);
				item3->setForeground(color);
				item3->setText(bp.condition.trimmed());

				QTableWidgetItem* item4 = bpTableWidget->item(bpRow, SLOT);
				item4->setForeground(color);
				item4->setText(QString("%1/%2")
					.arg(bp.ps == -1 ? QChar('X') : QChar('0' + bp.ps))
					.arg(bp.ss == -1 ? QChar('X') : QChar('0' + bp.ss)));

				QTableWidgetItem* item5 = bpTableWidget->item(bpRow, SEGMENT);
				item5->setForeground(color);
				item5->setText(bp.segment == -1 ? "X" : QString("%1").arg(bp.segment));

				items.append({BREAKPOINT, bpTableWidget->row(item5)});
				break;
			}
			case Breakpoint::WATCHPOINT_MEMREAD:
			case Breakpoint::WATCHPOINT_MEMWRITE:
			case Breakpoint::WATCHPOINT_IOREAD:
			case Breakpoint::WATCHPOINT_IOWRITE: {
				wpRow = createTableRow(WATCHPOINT);
				setBreakpointStatus(WATCHPOINT, wpRow, true);

				bool currentSlot = (bp.type == Breakpoint::WATCHPOINT_MEMREAD
				                 || bp.type == Breakpoint::WATCHPOINT_MEMWRITE)
				                  ? breakpoints->inCurrentSlot(bp) : true;
				QBrush color = palette().brush(runState | currentSlot ?
					QPalette::ColorGroup::Normal : QPalette::ColorGroup::Disabled,
					QPalette::WindowText);

				QTableWidgetItem* item1 = wpTableWidget->item(wpRow, ENABLED);
				item1->setCheckState(Qt::Checked);

				auto model = wpTableWidget->model();
				auto combo = (QComboBox*) wpTableWidget->indexWidget(model->index(wpRow, WP_TYPE));
				combo->setCurrentIndex(static_cast<int>(bp.type) - 1);
				int adrLen = (bp.type == Breakpoint::WATCHPOINT_IOREAD
				           || bp.type == Breakpoint::WATCHPOINT_IOWRITE) ? 2 : 4;

				QString address = hexValue(bp.address, adrLen);
				QString regionEnd = hexValue(bp.regionEnd, adrLen);
				QTableWidgetItem* item2 = wpTableWidget->item(wpRow, WP_REGION);
				item2->setForeground(color);
				item2->setText(QString("%1%2%3").arg(address)
						.arg(regionEnd == -1 || regionEnd == address ? "" : ":")
						.arg(regionEnd == -1 || regionEnd == address ? "" : regionEnd));

				QTableWidgetItem* item3 = wpTableWidget->item(wpRow, CONDITION);
				item3->setForeground(color);
				item3->setText(bp.condition.trimmed());

				QTableWidgetItem* item4 = wpTableWidget->item(wpRow, SLOT);
				item4->setForeground(color);
				item4->setText(QString("%1/%2")
					.arg(bp.ps == -1 ? QChar('X') : QChar('0' + bp.ps))
					.arg(bp.ss == -1 ? QChar('X') : QChar('0' + bp.ss)));

				QTableWidgetItem* item5 = wpTableWidget->item(wpRow, SEGMENT);
				item5->setForeground(color);
				item5->setText(bp.segment == -1 ? "X" : QString("%1").arg(bp.segment));

				items.append({WATCHPOINT, wpTableWidget->row(item5)});
				break;
			}
			case Breakpoint::CONDITION:
				if (!conditionsMsg) {
					QMessageBox::information(
						this,
						"Not yet implemented",
						"Sorry, the condition manager is not yet implemented");
					conditionsMsg = true;
				}
				break;
			default:
				assert(false);
			}
		}
	}

 	bpTableWidget->resizeColumnsToContents();
 	wpTableWidget->resizeColumnsToContents();
}

void BreakpointViewer::changeCurrentWpType(int row, int selected)
{
	if (!userMode) return;
	auto item = wpTableWidget->item(row, WP_TYPE);
	changeTableItem(WATCHPOINT, item);
}

void BreakpointViewer::createComboBox(int row)
{
	auto combo = new QComboBox();
	combo->setEditable(false);
	combo->insertItem(0, "write_io");
	combo->insertItem(0, "read_io");
	combo->insertItem(0, "write_mem");
	combo->insertItem(0, "read_mem");

	auto model = wpTableWidget->model();
	auto index = model->index(row, WP_TYPE);

	wpTableWidget->setIndexWidget(index, combo);

	connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		[this, row](int index){ changeCurrentWpType(row, index); });
}

int BreakpointViewer::createTableRow(Type type)
{
	auto sa = ScopedAssign(userMode, false);

	auto table = selector[type];
	int row = table->rowCount();
	table->setRowCount(row + 1);
	table->selectRow(row);

	QTableWidgetItem* item1 = new QTableWidgetItem();
	item1->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
	item1->setCheckState(Qt::Unchecked);
	table->setItem(row, ENABLED, item1);

	// type
	QTableWidgetItem* item2 = new QTableWidgetItem();
	item2->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, WP_TYPE, item2);
	if (type == WATCHPOINT) {
		createComboBox(row);
	}

	// address
	QTableWidgetItem* item3 = new QTableWidgetItem();
	item3->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, LOCATION, item3);

	// ps/ss
	QTableWidgetItem* item4 = new QTableWidgetItem();
	item4->setTextAlignment(Qt::AlignCenter);
	item4->setText("X/X");
	table->setItem(row, SLOT, item4);

	// segment
	QTableWidgetItem* item5 = new QTableWidgetItem();
	item5->setTextAlignment(Qt::AlignCenter);
	item5->setText("X");
	table->setItem(row, SEGMENT, item5);

	// condition
	QTableWidgetItem* item6 = new QTableWidgetItem();
	item6->setTextAlignment(Qt::AlignCenter);
	table->setItem(row, CONDITION, item6);

 	table->resizeColumnsToContents();

	return row;
}

void BreakpointViewer::on_btnAddBp_clicked()
{
	auto sa = ScopedAssign(userMode, false);
	int row = createTableRow(BREAKPOINT);
	setBreakpointStatus(BREAKPOINT, row, false);
}

void BreakpointViewer::on_btnRemoveBp_clicked()
{
	if (bpTableWidget->currentRow() == -1)
		return;

	auto sa = ScopedAssign(userMode, false);
	QTableWidgetItem* item = bpTableWidget->item(bpTableWidget->currentRow(), ENABLED);
	if (item->checkState() == Qt::Checked) {
		removeBreakpoint(BREAKPOINT, bpTableWidget->currentRow());
	}
	bpTableWidget->removeRow(bpTableWidget->currentRow());
	bpTableWidget->resizeColumnsToContents();
}

void BreakpointViewer::on_btnAddWp_clicked()
{
	auto sa = ScopedAssign(userMode, false);
	int row = createTableRow(WATCHPOINT);
	setBreakpointStatus(WATCHPOINT, row, false);
}

void BreakpointViewer::on_btnRemoveWp_clicked()
{
	if (wpTableWidget->currentRow() == -1)
		return;

	auto sa = ScopedAssign(userMode, false);
	QTableWidgetItem* item = wpTableWidget->item(wpTableWidget->currentRow(), ENABLED);
	if (item->checkState() == Qt::Checked) {
		removeBreakpoint(WATCHPOINT, wpTableWidget->currentRow());
	}
	wpTableWidget->removeRow(wpTableWidget->currentRow());
	wpTableWidget->resizeColumnsToContents();
}
