#ifndef BREAKPOINTDIALOG_OPENMSX_H
#define BREAKPOINTDIALOG_OPENMSX_H

#include "ui_BreakpointDialog.h"
#include "DebuggerData.h"
#include <QDialog>

struct MemoryLayout;

class DebugSession;
class Symbol;

class BreakpointDialog : public QDialog, private Ui::BreakpointDialog
{
	Q_OBJECT
public:
	BreakpointDialog(const MemoryLayout& ml, DebugSession *session = 0, QWidget* parent = 0);
	~BreakpointDialog();

	Breakpoints::Type type();
	int address();
	int addressEndRange();
	int slot();
	int subslot();
	int segment();
	QString condition();

	void setData(Breakpoints::Type type, int address = -1, 
	             int ps = -1, int ss = -1, int segment = -1,
	             int addressEnd = -1, QString condition = QString());

private:
	const MemoryLayout& memLayout;

	DebugSession *debugSession;
	Symbol *currentSymbol;
	int idxSlot, idxSubSlot;
	int value, valueEnd;
	int conditionHeight;
	QCompleter *jumpCompleter, *allCompleter;

private slots:
	void addressChanged(const QString& text);
	void typeChanged(int i);
	void slotChanged(int i);
	void subslotChanged(int i);
	void hasCondition(int state);
};

#endif // BREAKPOINTDIALOG_OPENMSX_H
