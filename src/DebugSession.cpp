// $Id$

#include "DebugSession.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMessageBox>

DebugSession::DebugSession()
	: modified(false)
{
}

DebugSession::~DebugSession()
{
}

bool DebugSession::existsAsFile() const
{
	return !fileName.isEmpty();
}

bool DebugSession::isModified() const
{
	return modified;
}

Breakpoints& DebugSession::breakpoints()
{
	return breaks;
}

SymbolTable& DebugSession::symbolTable()
{
	return symTable;
}

void DebugSession::clear()
{
	// clear everything
	symTable.clear();
	//breaks.clear();
	fileName.clear();
	modified = false;
}

void DebugSession::open( const QString& file )
{
	QFile f( file );
	// check open file
	if (!f.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(0, tr("Open session ..."),
		                      tr("Cannot read file %1:\n%2.")
		                      .arg(file)
		                      .arg(f.errorString()));
         return;
	}
	// clear current project
	clear();
	// start reading xml file
	QXmlStreamReader ses;
	ses.setDevice(&f);
	// read loop
	while (!ses.atEnd()) {
		ses.readNext();

		if (ses.isStartElement()) {
			if (ses.name() == "DebugSession" ) {
				// debug session data
				while( !ses.atEnd() ) {
					ses.readNext();

					// end tag
					if( ses.isEndElement() ) break;
					// begin tag
					if( ses.isStartElement() ) {
						if( ses.name() == "Symbols" )
							symTable.loadSymbols(ses);
						else if( ses.name() == "Breakpoints" )
							;// breaks.loadBreakpoints(ses)
						else
							skipUnknownElement(ses);
					}
				}
			}
		}
	}
	f.close();
	
	fileName = file;
	modified = false;
}

bool DebugSession::save()
{
	// open file for save
	QFile file( fileName );
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(0, tr("Save session ..."),
		                      tr("Cannot write file %1:\n%2.")
		                      .arg(fileName)
		                      .arg(file.errorString()));
         return false;
	}
	// start xml file
	QXmlStreamWriter ses;
	ses.setDevice(&file);
	ses.setAutoFormatting(true);
	ses.writeDTD("<!DOCTYPE xomds>");
	ses.writeStartElement("DebugSession");
	ses.writeAttribute("version", "0.1");
	// write symbols
	ses.writeStartElement("Symbols");
	symTable.saveSymbols(ses);
	ses.writeEndElement();
	// write breakpoints
	ses.writeStartElement("Breakpoints");
	//breaks.saveBreakpoints(ses);
	ses.writeEndElement();
	// end
	ses.writeEndDocument();
	file.close();
	modified = false;
	return true;
}

void DebugSession::saveAs( const QString& file )
{
	QString oldName = fileName;
	fileName = file;
	if( !save() ) {
		// save failed, restore old
		fileName = oldName;
	}
}

void DebugSession::skipUnknownElement( QXmlStreamReader& ses )
{
	while( !ses.atEnd() ) {
		ses.readNext();

		if( ses.isEndElement() ) break;

		if( ses.isStartElement() ) skipUnknownElement(ses);
	}
}

void DebugSession::sessionModified()
{
	modified = true;
}

