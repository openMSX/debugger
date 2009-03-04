// $Id$

#ifndef DEBUGSESSION_H
#define DEBUGSESSION_H

#include "DebuggerData.h"
#include "SymbolTable.h"
#include <QObject>

class QXmlStreamReader;

class DebugSession : public QObject
{
	Q_OBJECT
public:
	DebugSession();
	~DebugSession();

	// session
	void clear();
	void open(const QString& file);
	bool save();
	void saveAs(const QString& file);
	bool existsAsFile() const;
	const QString& filename() const;
	bool isModified() const;
	
	Breakpoints& breakpoints();
	SymbolTable& symbolTable();
	
private:
	QString fileName;
	bool modified;
	
	Breakpoints breaks;
	SymbolTable symTable;

	void skipUnknownElement(QXmlStreamReader& ses);
	
public slots:
	void sessionModified();
};

#endif // DEBUGSESSION_H
