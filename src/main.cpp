// $Id$

#include <QApplication>
#include "DebuggerForm.h"

int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );

	app.setStyle( "windows" );
	app.setFont(QFont("Verdana", 9));
	
    DebuggerForm debugger;
    debugger.show();

    return app.exec();
}
