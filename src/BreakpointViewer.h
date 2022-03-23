#ifndef BREAKPOINTVIEWER_H
#define BREAKPOINTVIEWER_H

#include "ui_BreakpointViewer.h"
#include "DebuggerData.h"
#include <QList>
#include <QTabWidget>

class QPaintEvent;
class Breakpoints;

class BreakpointViewer : public QTabWidget, private Ui::BreakpointViewer
{
	Q_OBJECT
public:
	BreakpointViewer(QWidget* parent = nullptr);
	void setBreakpoints(Breakpoints* bps);

	enum Type { BREAKPOINT, WATCHPOINT, CONDITION };

private:
	Ui::BreakpointViewer* ui;
	QTableWidget* selector[3];

	// layout
	int frameL, frameR, frameT, frameB;
	int leftRegPos, leftValuePos, rightRegPos, rightValuePos;
	int rowHeight;
	int cursorLoc;
	bool userMode;
	bool runState;
	bool conditionsMsg = false;
	Breakpoints* breakpoints;

	bool processLocationField(int index, BreakpointViewer::Type type, const QString& field,
		int& begin, int& end, const QString combo = QString());
	bool processSlotField(int index, const QString& field, qint8& ps, qint8& ss);
	bool processSegmentField(int index, const QString& field, qint16& segment);
	void changeTableItem(Type type, QTableWidgetItem* item);
	void createComboBox(int row);
	int createTableRow(Type type);

	void createBreakpoint(Type type, int row);
	void _createBreakpoint(Type type, int row);
	void _createCondition(int row);

	void replaceBreakpoint(Type type, int row);
	void removeBreakpoint(Type type, int row);
	void setBreakpointStatus(Type type, int row, int status);
	void keepUnusedItems();

private slots:
	void changeCurrentWpType(int row, int index);
	void changeBpTableItem(QTableWidgetItem* item);
	void changeWpTableItem(QTableWidgetItem* item);
	void changeCnTableItem(QTableWidgetItem* item);

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
