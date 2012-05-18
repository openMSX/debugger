#ifndef BREAKPOINTDIALOG_OPENMSX_H
#define BREAKPOINTDIALOG_OPENMSX_H

#include "ui_BreakpointDialog.h"
#include <QDialog>

struct MemoryLayout;

class DebugSession;
class Symbol;

class BreakpointDialog : public QDialog, private Ui::BreakpointDialog
{
	Q_OBJECT
public:
	BreakpointDialog(const MemoryLayout& ml, DebugSession *session = 0, QWidget* parent = 0);

	int address();
	int slot();
	int subslot();
	int segment();

private:
	const MemoryLayout& memLayout;

	DebugSession *debugSession;
	Symbol *currentSymbol;
	int idxSlot, idxSubSlot;

private slots:
	void addressChanged(const QString& text);
	void slotChanged(int i);
	void subslotChanged(int i);
};

#endif // BREAKPOINTDIALOG_OPENMSX_H
