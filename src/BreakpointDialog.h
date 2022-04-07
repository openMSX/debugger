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
	std::optional<AddressRange> addressRange(Symbol** symbol = nullptr) const;
	Slot slot() const;
	std::optional<uint8_t> segment() const;
	QString condition() const;

	void setData(Breakpoint::Type type, std::optional<AddressRange> range = {}, Slot slot = {},
	             std::optional<uint8_t> segment = {}, QString condition = QString());

private:
	const MemoryLayout& memLayout;

	DebugSession* debugSession;
	Symbol* currentSymbol;
	int idxSlot, idxSubSlot;
	int conditionHeight;
	std::unique_ptr<QCompleter> jumpCompleter;
	std::unique_ptr<QCompleter> allCompleter;
	std::optional<uint16_t> parseInput(QLineEdit* ed, Symbol** symbol = nullptr) const;
	void enableSlots();
	void disableSlots();

private slots:
	void addressChanged(const QString& text);
	void typeChanged(int i);
	void slotChanged(int i);
	void subslotChanged(int i);
	void hasCondition(int state);
};

#endif // BREAKPOINTDIALOG_OPENMSX_H
