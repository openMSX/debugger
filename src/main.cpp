// $Id$

#include "DebuggerForm.h"
#include <QApplication>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	DebuggerForm debugger;
	debugger.show();

	return app.exec();
}
