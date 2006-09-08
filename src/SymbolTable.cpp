// $Id$

#include "SymbolTable.h"
#include "DebuggerData.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>

/*
 * SymbolTable member functions
 */

SymbolTable::SymbolTable()
{
}

SymbolTable::~SymbolTable()
{
	addressSymbols.clear();
	valueSymbols.clear();
	qDeleteAll( symbols );
	symbols.clear();
}

void SymbolTable::add( Symbol *symbol )
{
	symbols.append( symbol );
	symbol->table = this;
	mapSymbol( symbol );
}

void SymbolTable::removeAt( int index )
{
	Symbol *symbol = symbols.takeAt( index );
	
	unmapSymbol( symbol );
}

void SymbolTable::mapSymbol( Symbol *symbol )
{
	if( symbol->type() != Symbol::VALUE ) {
		addressSymbols.insert( symbol->value(), symbol );
	}
	if( symbol->type() != Symbol::JUMPLABEL ) {
		valueSymbols.insert( symbol->value(), symbol );
	}
}

void SymbolTable::unmapSymbol( Symbol *symbol )
{
	QMutableMapIterator<int, Symbol*> i(addressSymbols);
	while (i.hasNext()) {
		i.next();
		if( i.value() == symbol ) i.remove();
	}
	QMutableHashIterator<int, Symbol*> j(valueSymbols);
	while (j.hasNext()) {
		j.next();
		if( j.value() == symbol ) j.remove();
	}
}

void SymbolTable::symbolTypeChanged( Symbol *symbol )
{
	unmapSymbol( symbol );
	mapSymbol( symbol );
}

void SymbolTable::symbolValueChanged( Symbol *symbol )
{
	unmapSymbol( symbol );
	mapSymbol( symbol );
}

Symbol *SymbolTable::findFirstAddressSymbol( int addr, MemoryLayout *ml )
{
	currentAddress = addressSymbols.begin();
	while( currentAddress != addressSymbols.end() ) {
		if( (*currentAddress)->value() >= addr ) {
			if( (*currentAddress)->isSlotValid( ml ) )
				return *currentAddress;
		}
		++currentAddress;
	}
	return 0;
}

Symbol *SymbolTable::getCurrentAddressSymbol()
{
	if( currentAddress == addressSymbols.end() )
		return 0;
	else
		return *currentAddress;
}

Symbol *SymbolTable::findNextAddressSymbol( MemoryLayout *ml )
{
	for(;;) {
		currentAddress++;
		if( currentAddress == addressSymbols.end() )
			return 0;
		else if( (*currentAddress)->isSlotValid(ml) )
			return *currentAddress;
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
		Symbol *sym = new Symbol( l.at(0), a.at(0).toInt(0, 16) );
		sym->setSource( symSource );
		add( sym );
	}
	return true;
}

bool SymbolTable::readASMSXFile( const QString& filename )
{
	QFile file( filename );
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QString *symSource = new QString(filename);
	symbolFiles.append(symSource);
	QTextStream in(&file);
	int filePart = 0;
	while (!in.atEnd()) {
		QString line = in.readLine();
		if( line[0] == ';' ) {
			if( line.startsWith("; global and local") ) {
				filePart = 1;
			} else if( line.startsWith("; other") ) {
				filePart = 2;
			}
		} else {
			if( line[0] == '$' ) {
				if( filePart == 1 ) {
					QStringList l = line.split(" ");
					Symbol *sym = new Symbol( l.at(1).trimmed(), l.at(0).right(4).toInt(0, 16) );
					sym->setSource( symSource );
					add( sym );
				} else if( filePart == 2 ) {
					
				}
			}
		}
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

		if( !keepSymbols ) {
			// remove symbols from address map
			QMutableMapIterator<int, Symbol*> mi(addressSymbols);
			while (mi.hasNext()) {
				mi.next();
				if( mi.value()->source() == name ) mi.remove();
			}
			// remove symbols from value hash
			QMutableHashIterator<int, Symbol*> hi(valueSymbols);
			while (hi.hasNext()) {
				hi.next();
				if( hi.value()->source() == name ) hi.remove();
			}
		}
		// remove symbols from value hash
		QMutableListIterator<Symbol*> i(symbols);
		while (i.hasNext()) {
			i.next();
			if( i.value()->source() == name ) 
				if( keepSymbols )
					i.value()->setSource(0);
				else
					i.remove();
		}
	}
}

/*
 * Symbol member functions
 */

Symbol::Symbol( const QString& str, int addr, int val ) 
	: symText( str ), symValue( addr ), symSlots( val )
{
	symStatus = ACTIVE;
	symType = JUMPLABEL;
	symSource = 0;
	symRegisters = 0;
	table = 0;
}

const QString& Symbol::text() const
{
	return symText;
}

void Symbol::setText( const QString& str )
{
	symText = str;
}

int Symbol::value() const
{
	return symValue;
}

void Symbol::setValue( int addr )
{
	if( addr != symValue ) {
		symValue = addr;
		if(table) table->symbolValueChanged( this );
	}
}

int Symbol::validSlots() const
{
	return symSlots;
}

void Symbol::setValidSlots( int val )
{
	symSlots = val & 0xFFFF;
}

int Symbol::validRegisters() const
{
	return symRegisters;
}

void Symbol::setValidRegisters( int regs )
{
	symRegisters = regs;
}

const QString *Symbol::source() const
{
	return symSource;
}

void Symbol::setSource( QString *name )
{
	symSource = name;
}

Symbol::SymbolStatus Symbol::status() const
{
	return symStatus;
}

void Symbol::setStatus( SymbolStatus s )
{
	symStatus = s;
}

Symbol::SymbolType Symbol::type() const
{
	return symType;
}

void Symbol::setType( SymbolType t )
{
	if( symType != t ) {
		symType = t;
		if( table ) table->symbolTypeChanged( this );
	}
}

bool Symbol::isSlotValid( MemoryLayout *ml )
{
	if( ml ) {
		int page = symValue >> 14;
		int ps = ml->primarySlot[page] & 3;
		int ss = 0;
		if( ml->isSubslotted[page] ) ss = ml->secondarySlot[page] & 3;
		if( symSlots & (1 << (4*ps+ss)) ) {
			if( symSegments.size() ) {
				for( int i = 0; i < symSegments.size(); i++ )
					if( ml->mapperSegment[page] == symSegments[i] )
						return true;
			} else
				return true;
		}
	} else
		return true;
	return false;
}
