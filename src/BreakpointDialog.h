#ifndef BREAKPOINTDIALOG_OPENMSX_H
#define BREAKPOINTDIALOG_OPENMSX_H

#include "ui_BreakpointDialog.h"
#include "DebuggerData.h"
#include <QCompleter>
#include <QDialog>
#include <memory>

struct MemoryLayout;

class DebugSession;
class Symbol;

class BreakpointDialog : public QDialog, private Ui::BreakpointDialog
{
	Q_OBJECT
public:
	BreakpointDialog(const MemoryLayout& ml, DebugSession *session = nullptr, QWidget* parent = nullptr);

	Breakpoint::Type type() const;
	int address() const;
	int addressEndRange() const;
	int slot() const;
	int subslot() const;
	int segment() const;
	QString condition() const;

	void setData(Breakpoint::Type type, int address = -1,
	             qint8 ps = -1, qint8 ss = -1, qint16 segment = -1,
	             int addressEnd = -1, QString condition = QString());

private:
	const MemoryLayout& memLayout;

	DebugSession *debugSession;
	Symbol *currentSymbol;
	int idxSlot, idxSubSlot;
	int value, valueEnd;
	int conditionHeight;
	std::unique_ptr<QCompleter> jumpCompleter;
	std::unique_ptr<QCompleter> allCompleter;

private slots:
	void addressChanged(const QString& text);
	void typeChanged(int i);
	void slotChanged(int i);
	void subslotChanged(int i);
	void hasCondition(int state);
};

#endif // BREAKPOINTDIALOG_OPENMSX_H
