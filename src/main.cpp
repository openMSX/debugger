// $Id$

#include "DebuggerForm.h"
#include "Settings.h"
#include <QApplication>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	// restore main settings
	app.setFont( Settings::get().font( Settings::APP_FONT ) );
	
	DebuggerForm debugger;
	debugger.show();

	return app.exec();
}
