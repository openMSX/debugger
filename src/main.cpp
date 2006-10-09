// $Id$

#include "DebuggerForm.h"
#include "Settings.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	app.setWindowIcon(QIcon::QIcon(":icons/logo.png"));
	// restore main settings
	app.setFont( Settings::get().font( Settings::APP_FONT ) );
	
	DebuggerForm debugger;
	debugger.show();

	return app.exec();
}
