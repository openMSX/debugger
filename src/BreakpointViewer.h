#ifndef BREAKPOINTVIEWER_H
#define BREAKPOINTVIEWER_H

#include "ui_BreakpointViewer.h"
#include "DebuggerData.h"
#include <QList>
#include <QTabWidget>
#include <optional>
#include <tuple>

class QPaintEvent;
class Breakpoints;

struct BreakpointRef {
	enum Type { BREAKPOINT, WATCHPOINT, CONDITION, ALL } type;

	QString id;

	int row = -1;
	int index = -1;
};

class BreakpointViewer : public QTabWidget, private Ui::BreakpointViewer
{
	Q_OBJECT
public:
	BreakpointViewer(QWidget* parent = nullptr);
	void setBreakpoints(Breakpoints* bps);

	void on_btnAddBp_clicked();
	void on_btnRemoveBp_clicked();
	void on_btnAddWp_clicked();
	void on_btnRemoveWp_clicked();
	void on_btnAddCn_clicked();
	void on_btnRemoveCn_clicked();

	void setRunState();
	void setBreakState();
	void sync();

signals:
	void contentsUpdated(bool merge);

private:
	void setTextField(BreakpointRef::Type type, int row, int column, const QString& value);
	std::optional<AddressRange> parseLocationField(std::optional<int> index,
	                                               BreakpointRef::Type type,
	                                               const QString& field,
	                                               const QString& combo = {});
	Slot parseSlotField(std::optional<int> index, const QString& field);
	std::optional<uint8_t> parseSegmentField(std::optional<int> index, const QString& field);
	void changeTableItem(BreakpointRef::Type type, QTableWidgetItem* item);
	void createComboBox(int row);
	Breakpoint::Type readComboBox(int row);
	int  createTableRow(BreakpointRef::Type type, int row = -1);
	void fillTableRow(int index, BreakpointRef::Type type, int row);
	std::optional<Breakpoint> parseTableRow(BreakpointRef::Type type, int row);
	bool connectBreakpointID(const QString& id, BreakpointRef& data);
	void populate();

	void createBreakpoint(BreakpointRef::Type type, int row);
	void _handleSyncError(const QString& error);
	void _handleKeyAlreadyExists();
	void _handleKeyNotFound();
	void _createBreakpoint(BreakpointRef::Type type, int row);
	void _createCondition(int row);

	void replaceBreakpoint(BreakpointRef::Type type, int row);
	void removeBreakpoint(BreakpointRef::Type type, int row, bool logical = false);
	void setBreakpointChecked(BreakpointRef::Type type, int row, Qt::CheckState state);
	void onAddBtnClicked(BreakpointRef::Type type);
	void onRemoveBtnClicked(BreakpointRef::Type type);
	void stretchTable(BreakpointRef::Type type = BreakpointRef::ALL);

	BreakpointRef* scanBreakpointRef(BreakpointRef::Type type, int row);
	BreakpointRef* findBreakpointRef(BreakpointRef::Type type, int row);
	BreakpointRef* scanBreakpointRef(const Breakpoint& bp);

	void changeCurrentWpType(int row, int index);
	void disableSorting(BreakpointRef::Type type = BreakpointRef::ALL);
	void changeBpTableItem(QTableWidgetItem* item);
	void changeWpTableItem(QTableWidgetItem* item);
	void changeCnTableItem(QTableWidgetItem* item);
	void on_itemPressed(QTableWidgetItem* item);
	void on_headerClicked(int index);

private:
	Ui::BreakpointViewer* ui;
	QTableWidget* tables[BreakpointRef::ALL];
	std::map<QString, BreakpointRef> maps[BreakpointRef::ALL];

	bool selfUpdating = false;
	bool userMode = true;
	bool runState;
	bool conditionsMsg = false;
	Breakpoints* breakpoints;
};

#endif // BREAKPOINTVIEWER_H
