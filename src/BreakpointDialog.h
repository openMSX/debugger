#ifndef BREAKPOINTDIALOG_OPENMSX_H
#define BREAKPOINTDIALOG_OPENMSX_H

#include "ui_BreakpointDialog.h"

struct MemoryLayout;

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

#endif /* BREAKPOINTDIALOG_OPENMSX_H */
