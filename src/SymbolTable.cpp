// $Id$

#include "SymbolTable.h"
#include "DebuggerData.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>

/*
 * AddressSymbol member functions
 */

AddressSymbol::AddressSymbol( const QString& str, int addr, int val ) 
	: text( str ), address( addr ), validSlots( val )
{
	symbolStatus = ACTIVE;
	source = 0;
}

const QString& AddressSymbol::getText() const
{
	return text;
}

void AddressSymbol::setText( const QString& str )
{
	text = str;
}

int AddressSymbol::getAddress() const
{
	return address;
}

void AddressSymbol::setAddress( int addr )
{
	address = addr;
}

const int AddressSymbol::getValidSlots() const
{
	return validSlots;
}

void AddressSymbol::setValidSlots( int val )
{
	validSlots = val & 0xFFFF;
}

const QString *AddressSymbol::getSource() const
{
	return source;
}

void AddressSymbol::setSource( QString *name )
{
	source = name;
}

AddressSymbol::SymbolStatus AddressSymbol::status() const
{
	return symbolStatus;
}

void AddressSymbol::setStatus( SymbolStatus s )
{
	symbolStatus = s;
}

bool AddressSymbol::isSlotValid( MemoryLayout *ml )
{
	if( ml ) {
		int page = address >> 14;
		int ps = ml->primarySlot[page] & 3;
		int ss = 0;
		if( ml->isSubslotted[page] ) ss = ml->secondarySlot[page] & 3;
		if( validSlots & (1 << (4*ps+ss)) ) {
			if( validSegments.size() ) {
				for( int i = 0; i < validSegments.size(); i++ )
					if( ml->mapperSegment[page] == validSegments[i] )
						return true;
			} else
				return true;
		}
	} else
		return true;
	return false;
}

/*
 * SymbolTable member functions
 */

SymbolTable::SymbolTable()
{
}

SymbolTable::~SymbolTable()
{
	qDeleteAll( addressSymbols );
	addressSymbols.clear();
}

void SymbolTable::addAddressSymbol( AddressSymbol *symbol )
{
	QList<AddressSymbol*>::iterator i;
	for( i = addressSymbols.begin(); i != addressSymbols.end(); ++i )
		if( (*i)->getAddress() > symbol->getAddress() ) {
			addressSymbols.insert( i, symbol );
			return;
		}
	addressSymbols.append( symbol );
}

void SymbolTable::removeAddressSymbolAt( int index )
{
	delete addressSymbols[index];
	addressSymbols.removeAt(index);
}

AddressSymbol *SymbolTable::findFirstAddressSymbol( int addr, MemoryLayout *ml )
{
	currentSymbol = addressSymbols.begin();
	while( currentSymbol != addressSymbols.end() ) {
		if( (*currentSymbol)->getAddress() >= addr ) {
			if( (*currentSymbol)->isSlotValid( ml ) )
				return *currentSymbol;
		}
		++currentSymbol;
	}
	return 0;
}

AddressSymbol *SymbolTable::getCurrentAddressSymbol()
{
	if( currentSymbol == addressSymbols.end() )
		return 0;
	else
		return *currentSymbol;
}

AddressSymbol *SymbolTable::findNextAddressSymbol( MemoryLayout *ml )
{
	for(;;) {
		currentSymbol++;
		if( currentSymbol == addressSymbols.end() )
			return 0;
		else if( (*currentSymbol)->isSlotValid(ml) )
			return *currentSymbol;
	}
}

int SymbolTable::symbolFilesSize() const
{
	return symbolFiles.size();
}

const QString& SymbolTable::symbolFile( int index ) const
{
	return *symbolFiles.at(index);
}

bool SymbolTable::readTNIASM0File( const QString& filename )
{
	QFile file( filename );
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QString *symSource = new QString(filename);
	symbolFiles.append(symSource);
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split( ": equ " );
		if( l.size() != 2 ) continue;
		QStringList a = l.at(1).split( "h ;" );
		if( a.size() != 2 ) continue;
		AddressSymbol *sym = new AddressSymbol( l.at(0), a.at(0).toInt(0, 16) );
		sym->setSource( symSource );
		addAddressSymbol( sym );
	}
	return true;
}

void SymbolTable::reloadFiles()
{
	
}

void SymbolTable::unloadFile( const QString& file, bool keepSymbols )
{
	int index = -1;
	for( int i = 0; i < symbolFiles.size(); i++ )
		if( *symbolFiles[i] == file ) {
			index = i;
			break;
		}
	
	if( index >= 0 ) {
		QString *name = symbolFiles.takeAt(index);
		int i = 0;
		while( i < addressSymbols.size() ) {
			AddressSymbol *sym = addressSymbols[i];
			if( sym->getSource() == name ) {
				if( keepSymbols ) {
					sym->setSource(0);
					i++;
				} else {
					addressSymbols.removeAt( i );
					delete sym;
				}
			} else i++;
		}
	}
}
