// $Id$

#include "DebuggerForm.h"
#include "Settings.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
// Don't set the icon on OS X, because it will replace the high-res version
// with a lower resolution one, even though openMSX-debugger-logo-256.png is 256x256.
#ifndef __APPLE__
	app.setWindowIcon(QIcon(":icons/openMSX-debugger-logo-256.png"));
#endif
	// restore main settings
	app.setFont(Settings::get().font(Settings::APP_FONT));

	DebuggerForm debugger;
	debugger.show();

	return app.exec();
}
