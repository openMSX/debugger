#include "GotoDialog.h"
#include "DebugSession.h"
#include "Convert.h"
#include <QCompleter>


GotoDialog::GotoDialog(const MemoryLayout& ml, DebugSession *session, QWidget* parent)
	: QDialog(parent), memLayout(ml), currentSymbol(0)
{
	setupUi(this);

	debugSession = session;
	if( session ) {
		// create address completer
		QCompleter *completer = new QCompleter(session->symbolTable().labelList(true, &ml), this);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		edtAddress->setCompleter(completer);
		connect(completer,  SIGNAL(activated(const QString&)), this, SLOT(addressChanged(const QString&)));
	}

	connect(edtAddress,  SIGNAL(textEdited(const QString&)), this, SLOT(addressChanged(const QString&)));
}

int GotoDialog::address()
{
	if(currentSymbol)
		return currentSymbol->value();
	else
		return stringToValue(edtAddress->text());
}

void GotoDialog::addressChanged(const QString& text)
{
	int addr = stringToValue(text);
	if (addr == -1 && debugSession) {
		// try finding a label
		currentSymbol = debugSession->symbolTable().getAddressSymbol(text);
		if (!currentSymbol) currentSymbol = debugSession->symbolTable().getAddressSymbol(text, Qt::CaseInsensitive);
		if (currentSymbol) addr = currentSymbol->value();
	}

	QPalette pal;
	pal.setColor(QPalette::Text, addr==-1 ? Qt::red : Qt::black);
	edtAddress->setPalette(pal);
}

