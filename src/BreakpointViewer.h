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
class DebugSession;

enum BreakpointType { BREAKPOINT, WATCHPOINT, CONDITION, ALL };

struct BreakpointRef {
	enum BreakpointType type;
	QString id;
	int tableIndex = -1;
	int breakpointIndex = -1;

	bool operator==(const QString &id_) const {
		return id_ == id;
	}
};

class BreakpointViewer : public QTabWidget, private Ui::BreakpointViewer
{
	Q_OBJECT
public:
	BreakpointViewer(DebugSession& session, QWidget* parent = nullptr);
	void setBreakpoints(Breakpoints* bps);

	void onSymbolTableChanged();
	void on_btnAddBp_clicked();
	void on_btnRemoveBp_clicked();
	void on_btnAddWp_clicked();
	void on_btnRemoveWp_clicked();
	void on_btnAddCn_clicked();
	void on_btnRemoveCn_clicked();

	void setRunState();
	void setBreakState();
	void refresh();

signals:
	void contentsUpdated();

private:
	void setTextField(BreakpointType type, int row, int column, const QString& value, const QString& tooltip = {});

	std::optional<AddressRange> parseSymbolOrValue(const QString& field) const;

	QString findSymbolOrValue(uint16_t address) const;

	std::optional<AddressRange> parseLocationField(std::optional<int> bpIndex,
	                                               BreakpointType type,
	                                               const QString& field,
	                                               const QString& combo = {});
	Slot parseSlotField(std::optional<int> bpIndex, const QString& field);
	std::optional<uint8_t> parseSegmentField(std::optional<int> bpIndex, const QString& field);
	void changeTableItem(BreakpointType type, QTableWidgetItem* item);
	void createComboBox(int row);
	Breakpoint::Type readComboBox(int row);
	int  createTableRow(BreakpointType type, int row = -1);
	void fillTableRowLocation(BreakpointType type, int row, const QString& location);
	void fillTableRow(BreakpointType type, int row, int bpIndex);
	std::optional<Breakpoint> parseTableRow(BreakpointType type, int row);
	bool addBreakpointRef(const QString& id, BreakpointRef& data);

	std::optional<int> getTableIndexByRow(BreakpointType type, int row) const;
	void createBreakpoint(BreakpointType type, int row);
	void _handleSyncError(const QString& error);
	void _handleKeyAlreadyExists();
	void _handleKeyNotFound();
	void _createBreakpoint(BreakpointType type, int row);
	void _createCondition(int row);

	void replaceBreakpoint(BreakpointType type, int row);
	void removeBreakpoint(BreakpointType type, int row, bool removeLocal = false);
	void setBreakpointChecked(BreakpointType type, int row, Qt::CheckState state);
	void onAddBtnClicked(BreakpointType type);
	void onRemoveBtnClicked(BreakpointType type);
	void stretchTable(BreakpointType type = BreakpointType::ALL);

	std::optional<int> findTableRowByIndex(BreakpointType type, int index) const;
	BreakpointRef* findBreakpointRef(BreakpointType type, int row);
	BreakpointRef* findBreakpointRefById(BreakpointType type, const QString& id);

	void changeCurrentWpType(int row, int index);
	void disableSorting(BreakpointType type = BreakpointType::ALL);
	void changeBpTableItem(QTableWidgetItem* item);
	void changeWpTableItem(QTableWidgetItem* item);
	void changeCnTableItem(QTableWidgetItem* item);
	void on_itemPressed(QTableWidgetItem* item);
	void on_headerClicked(int index);

private:
	Ui::BreakpointViewer* ui;
	DebugSession& debugSession;

	QTableWidget* tables[BreakpointType::ALL];
	std::map<QString, BreakpointRef> maps[BreakpointType::ALL];

	bool disableRefresh = false;
	bool userMode = true;
	bool runState;
	bool conditionsMsg = false;
	Breakpoints* breakpoints;
};

#endif // BREAKPOINTVIEWER_H
