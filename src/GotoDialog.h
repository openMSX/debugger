#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include "ui_GotoDialog.h"
#include <QDialog>
#include <optional>

struct MemoryLayout;

class DebugSession;
class Symbol;

class GotoDialog : public QDialog, private Ui::GotoDialog
{
	Q_OBJECT
public:
	GotoDialog(const MemoryLayout& ml, DebugSession* session = nullptr, QWidget* parent = nullptr);

	std::optional<uint16_t> address();

private:
	const MemoryLayout& memLayout;

	DebugSession* debugSession;
	Symbol* currentSymbol;

private slots:
	void addressChanged(const QString& text);
};

#endif // GOTODIALOG_H
