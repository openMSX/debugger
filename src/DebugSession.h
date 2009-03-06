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

	// session
	void clear();
	void open(const QString& file);
	bool save();
	bool saveAs(const QString& file);
	bool existsAsFile() const;
	const QString& filename() const;
	bool isModified() const;

	Breakpoints& breakpoints();
	SymbolTable& symbolTable();
	
private:
	void skipUnknownElement(QXmlStreamReader& ses);

	Breakpoints breaks;
	SymbolTable symTable;
	QString fileName;
	bool modified;

public slots:
	void sessionModified();
};

#endif // DEBUGSESSION_H
