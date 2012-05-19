#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include "ui_GotoDialog.h"
#include <QDialog>

struct MemoryLayout;

class DebugSession;
class Symbol;

class GotoDialog : public QDialog, private Ui::GotoDialog
{
	Q_OBJECT
public:
	GotoDialog(const MemoryLayout& ml, DebugSession *session = 0, QWidget* parent = 0);

	int address();

private:
	const MemoryLayout& memLayout;

	DebugSession *debugSession;
	Symbol *currentSymbol;

private slots:
	void addressChanged(const QString& text);
};

#endif // GOTODIALOG_H
