// $Id$

#include "SymbolTable.h"
#include "DebuggerData.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegExp>
#include <QFileInfo>


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
	delete symbol;
}

void SymbolTable::remove( Symbol *symbol )
{
	symbols.removeAll( symbol );
	unmapSymbol( symbol );
	delete symbol;
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

Symbol *SymbolTable::getValueSymbol( int val, Symbol::Register reg, MemoryLayout *ml )
{
	QMultiHash<int, Symbol*>::iterator it = valueSymbols.find( val );
	while( it != valueSymbols.end() &&  it.key() == val ) {
		if( it.value()->validRegisters() & reg )
			if( it.value()->isSlotValid( ml ) )
				return it.value();
		it++;
	}
	return 0;
}

Symbol *SymbolTable::getAddressSymbol( int addr, MemoryLayout *ml )
{
	QMultiMap<int, Symbol*>::iterator it = addressSymbols.find( addr );
	while( it != addressSymbols.end() && it.key() == addr ) {
		if( it.value()->isSlotValid( ml ) )
			return it.value();
		it++;
	}
	return 0;
}

int SymbolTable::symbolFilesSize() const
{
	return symbolFiles.size();
}

const QString& SymbolTable::symbolFile( int index ) const
{
	return symbolFiles.at(index).fileName;
}

const QDateTime& SymbolTable::symbolFileRefresh( int index ) const
{
	return symbolFiles.at(index).refreshTime;
}

bool SymbolTable::readFile( const QString& filename, FileType type )
{
	if( type == DETECT_FILE ) {
		if ( filename.endsWith(".map") ) {
			// HiTech link map file
			type = LINKMAP_FILE;
		} else if( filename.endsWith(".sym") ) {
			// auto detect which sym file
			QFile file( filename );
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream in(&file);
				QString line = in.readLine();
				file.close();
				if( line[0] == ';' )
					type = ASMSX_FILE;
				else if( line.contains( ": equ " ) )
					type = TNIASM_FILE;
			}
		}
	}
		
	switch( type ) {
		case TNIASM_FILE:
			return readTNIASM0File( filename );
		case ASMSX_FILE:
			return readASMSXFile( filename );
		case LINKMAP_FILE:
			return readLinkMapFile( filename );
		default:
			return false;
	}	
	
}

void SymbolTable::appendFile( const QString& file, FileType type )
{
	SymbolFileRecord rec;
	rec.fileName = file;
	rec.fileType = type;
	rec.refreshTime = QDateTime::currentDateTime();
	symbolFiles.append(rec);
}

bool SymbolTable::readTNIASM0File( const QString& filename )
{
	QFile file( filename );

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	appendFile(filename, TNIASM_FILE);
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split( ": equ " );
		if( l.size() != 2 ) continue;
		QStringList a = l.at(1).split( "h ;" );
		if( a.size() != 2 ) continue;
		Symbol *sym = new Symbol( l.at(0), a.at(0).toInt(0, 16) );
		sym->setSource( &symbolFiles.back().fileName );
		add( sym );
	}
	return true;
}

bool SymbolTable::readASMSXFile( const QString& filename )
{
	QFile file( filename );

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	appendFile(filename, ASMSX_FILE);
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
			if(( line[0] == '$' ) || ( line[4] == 'h' ) || ( line[5] == 'h' ) || ( line[8] == 'h' )) {
				if( filePart == 1 ) {
					QStringList l = line.split(" ");
					Symbol *sym;
					if( line[0] == '$' ) {
						sym = new Symbol( l.at(1).trimmed(), l.at(0).right(4).toInt(0, 16) );
					} else if(( line[4] == 'h' ) || ( line[5] == 'h' )) {
						sym = new Symbol( l.at(1).trimmed(), l.at(0).mid( l.at(0).indexOf('h') - 4,4 ).toInt(0, 16) );
					} else {
						QString m = l.at(0);
						QStringList n = m.split(":"); // n.at(0) = MegaROM page
						sym = new Symbol( l.at(1).trimmed(), n.at(1).left(4).toInt(0, 16) );
					}
					sym->setSource( &symbolFiles.back().fileName );
					add( sym );
				} else if( filePart == 2 ) {

				}
			}
		}
	}
	return true;
}

bool SymbolTable::readLinkMapFile( const QString& filename )
{
	QFile file( filename );
	QString magic("Machine type");
	QString tableStart("*\tSymbol Table");
	QRegExp rx(" [0-9A-Fa-f]{4}  (?![ 0-9])");
	QRegExp rp("^([^ ]+) +[^ ]* +([0-9A-Fa-f]{4})  $");

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	appendFile(filename, LINKMAP_FILE);
	QTextStream in(&file);
	QString line;
	if (! in.atEnd()) {
		line = in.readLine();
		if (!line.startsWith(magic))
			return false;
	} else return false;
	while (!in.atEnd()) {
		line = in.readLine();
		if (line.startsWith(tableStart))
			break;
	}
	if (!line.startsWith(tableStart))
		return false;

	while (! in.atEnd()) {
		line = in.readLine();
		Q_ASSERT(!line.endsWith("\r"));
		if (0 == line.length())
			continue;
		line += "  ";
		int len = line.length();
		int l;
		int pos = 0;
		bool ok = false;
		// HiTech uses multiple columns of non-fixed width and
		// a column for psect may be blank so the address may in the first or second match.
		for (int tries = 0 ; (tries < 2) && ! ok ; tries ++) {
			pos = rx.indexIn(line, pos);
			l = pos + rx.matchedLength();
			if ((l>0) && (len % l) == 0) {
				ok = true;
				for (int posn = pos+l; (posn < len) && ok ; posn += l) {
					ok = (posn == rx.indexIn(line, posn));
				}
			}
			pos = l-1;
		}
		if (! ok) continue;

		for (pos = 0 ; pos <len ; pos+= l) {
			QString part = line.mid( pos, l );
			if ( 0 == rp.indexIn( part ) ) {
				QStringList l = rp.capturedTexts();
				Symbol *sym =new Symbol( l.at(1), l.last().toInt(0, 16) );
				sym->setSource( &symbolFiles.back().fileName );
				add( sym );
			}
		}
	}
	return true;
}

void SymbolTable::reloadFiles()
{
	for( int i = 0; i < symbolFiles.size(); i++ ) {
		// check if file is newer
		QFileInfo fi = QFileInfo(symbolFiles[i].fileName);
		if( fi.lastModified() > symbolFiles[i].refreshTime ) {
			// file info
			QString name = symbolFiles[i].fileName;
			FileType type = symbolFiles[i].fileType;
			// symbol file is newer
			QMap<QString, Symbol> symCopy;
			// copy all symbols originating from this file
			QMutableListIterator<Symbol*> si(symbols);
			while (si.hasNext()) {
				si.next();
				if( si.value()->source() == &symbolFiles[i].fileName )
					symCopy.insert( si.value()->text(), Symbol(*(si.value())) );
			}
			// remove existing file
			unloadFile( name );
			// read the new file
			readFile( name, type );
			// find old symbols in newly loaded file
			QMutableListIterator<Symbol*> ni(symbols);
			QString *newFile = &symbolFiles.back().fileName;
			while (ni.hasNext()) {
				ni.next();
				if( ni.value()->source() == newFile ) {
					// find symbol in old list
					QMap<QString, Symbol>::iterator sit = symCopy.find( ni.value()->text() );
					if( sit != symCopy.end() ) {
						// symbol existed before, copy settings
						ni.value()->setValidSlots( sit->validSlots() );
						ni.value()->setValidRegisters( sit->validRegisters() );
						ni.value()->setType( sit->type() );
						if( sit->status() == Symbol::LOST )
							ni.value()->setStatus( Symbol::ACTIVE );
						else
							ni.value()->setStatus( sit->status() );
						symCopy.erase(sit);
					}
				}
			}
			// all symbols left in map are lost
			QMap<QString, Symbol>::iterator sit = symCopy.begin();
			while( sit != symCopy.end() ) {
				Symbol *sym = new Symbol( sit.value() );
				sym->setStatus( Symbol::LOST );
				sym->setSource( newFile );
				add(sym);
				sit++;
			}
		}
	}
}

void SymbolTable::unloadFile( const QString& file, bool keepSymbols )
{
	int index = -1;
	for( int i = 0; i < symbolFiles.size(); i++ )
		if( symbolFiles[i].fileName == file ) {
			index = i;
			break;
		}

	if( index >= 0 ) {
		QString *name = &symbolFiles[index].fileName;

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
			Symbol *sym = i.value();
			if( sym->source() == name )
				if( keepSymbols ) {
					sym->setSource(0);
				} else {
					i.remove();
					delete sym;
				}
		}
		// remove record
		symbolFiles.removeAt(index);
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
	if( val > 255 )
		symRegisters = REG_ALL16;
	else
		symRegisters = REG_ALL;
	table = 0;
}

Symbol::Symbol( const Symbol& symbol )
{
	table = 0;
	symStatus = symbol.symStatus;
	symText = symbol.symText;
	symValue = symbol.symValue;
	symSlots = symbol.symSlots;
	symSegments = symbol.symSegments;
	symRegisters = symbol.symRegisters;
	symSource = symbol.symSource;
	symStatus = symbol.symStatus;
	symType = symbol.symType;
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
