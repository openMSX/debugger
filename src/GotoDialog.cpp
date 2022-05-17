#include "GotoDialog.h"
#include "DebugSession.h"
#include "Convert.h"
#include <QCompleter>


GotoDialog::GotoDialog(const MemoryLayout& ml, DebugSession *session, QWidget* parent)
	: QDialog(parent), memLayout(ml), currentSymbol(nullptr)
{
	setupUi(this);

	debugSession = session;
	if (session) {
		// create address completer
		auto* completer = new QCompleter(session->symbolTable().labelList(true, &ml), this);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		edtAddress->setCompleter(completer);
		connect(completer,  qOverload<const QString&>(&QCompleter::activated), this, &GotoDialog::addressChanged);
	}

	connect(edtAddress, &QLineEdit::textEdited, this, &GotoDialog::addressChanged);
}

std::optional<uint16_t> GotoDialog::address()
{
	return currentSymbol ? currentSymbol->value()
	                     : stringToValue<int>(edtAddress->text());
}

void GotoDialog::addressChanged(const QString& text)
{
	auto addr = stringToValue<uint16_t>(text);

	if (addr && debugSession) {
		// try finding a label
		currentSymbol = debugSession->symbolTable().getAddressSymbol(text);
		if (!currentSymbol) currentSymbol = debugSession->symbolTable().getAddressSymbol(text, Qt::CaseInsensitive);
		if (currentSymbol) addr = currentSymbol->value();
	}

	QPalette pal;
	pal.setColor(QPalette::Text, addr ? Qt::black : Qt::red);
	edtAddress->setPalette(pal);
}
