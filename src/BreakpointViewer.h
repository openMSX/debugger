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

struct BreakpointData {
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

private:
	Ui::BreakpointViewer* ui;
	QTableWidget* tables[BreakpointData::ALL];
	std::map<QString, BreakpointData> maps[BreakpointData::ALL];

	bool selfUpdating = false;
	bool userMode = true;
	bool runState;
	bool conditionsMsg = false;
	Breakpoints* breakpoints;

	void setTextField(BreakpointData::Type type, int row, int column, const QString& value);
	std::optional<std::tuple<int, int>> parseLocationField(int index, BreakpointData::Type type, const QString& field,
		                                               const QString& combo = {});
	std::optional<std::tuple<qint8, qint8>> parseSlotField(int index, const QString& field);
	std::optional<qint16> parseSegmentField(int index, const QString& field);
	void changeTableItem(BreakpointData::Type type, QTableWidgetItem* item);
	void createComboBox(int row);
	Breakpoint::Type readComboBox(int row);
	int  createTableRow(BreakpointData::Type type, int row = -1);
	void fillTableRow(int index, BreakpointData::Type type, int row);
	bool connectBreakpointID(const QString& id, BreakpointData& data);
	void populate();

	void createBreakpoint(BreakpointData::Type type, int row);
	void _handleSyncError(const QString& error);
	void _handleKeyAlreadyExists();
	void _handleKeyNotFound();
	void _createBreakpoint(BreakpointData::Type type, int row);
	void _createCondition(int row);

	void replaceBreakpoint(BreakpointData::Type type, int row);
	void removeBreakpoint(BreakpointData::Type type, int row, bool logical = false);
	void setBreakpointChecked(BreakpointData::Type type, int row, Qt::CheckState state);
	void onAddBtnClicked(BreakpointData::Type type);
	void onRemoveBtnClicked(BreakpointData::Type type);
	void stretchTable(BreakpointData::Type type = BreakpointData::ALL);

	std::map<QString, BreakpointData>::iterator scanBreakpointData(const Breakpoint& bp);
	std::map<QString, BreakpointData>::iterator findBreakpointData(BreakpointData::Type type, const QString& id);

private slots:
	void changeCurrentWpType(int row, int index);
	void disableSorting(BreakpointData::Type type = BreakpointData::ALL);
	void changeBpTableItem(QTableWidgetItem* item);
	void changeWpTableItem(QTableWidgetItem* item);
	void changeCnTableItem(QTableWidgetItem* item);
	void on_itemPressed(QTableWidgetItem* item);
	void on_headerClicked(int index);

public slots:
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
};

#endif // BREAKPOINTVIEWER_H
