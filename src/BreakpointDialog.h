#ifndef BREAKPOINTDIALOG_H
#define BREAKPOINTDIALOG_H

#include "ui_BreakpointDialog.h"

class MemoryLayout;

class BreakpointDialog : public QDialog, private Ui::BreakpointDialog
{
	Q_OBJECT
public:
	BreakpointDialog(const MemoryLayout& ml, QWidget *parent = 0);

	int address();
	int slot();
	int subslot();
	int segment();

private:
	const MemoryLayout& memLayout;

private slots:
	void slotChanged( int i );
	void subslotChanged( int i );
};

#endif /* BREAKPOINTDIALOG_H */
