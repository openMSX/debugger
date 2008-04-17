// $Id$
#ifndef _DEBUGSESSION_H
#define _DEBUGSESSION_H

#include <QObject>
#include "DebuggerData.h"
#include "SymbolTable.h"

class QXmlStreamReader;

class DebugSession : public QObject
{
	Q_OBJECT
public:
	DebugSession();
	~DebugSession();

	// session
	void clear();
	void open( const QString& file );
	bool save();
	void saveAs( const QString& file );
	bool existsAsFile() const;
	bool isModified() const;
	
	Breakpoints& breakpoints();
	SymbolTable& symbolTable();
	
private:
	QString fileName;
	bool modified;
	
	Breakpoints breaks;
	SymbolTable symTable;

	void skipUnknownElement( QXmlStreamReader& ses );
	
public slots:
	void sessionModified();
	
};



#endif // _DEBUGSESSION_H
