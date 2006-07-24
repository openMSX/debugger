// $Id$
#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <QString>
#include <QList>

struct MemoryLayout;

class AddressSymbol
{
public:
	AddressSymbol( const QString& str, int addr, int val = 0xFFFF );
	
	enum SymbolStatus { ACTIVE, HIDDEN, LOST };
	
	const QString& getText() const;
	void setText( const QString& str );
	int getAddress() const;
	void setAddress( int addr );
	const int getValidSlots() const;
	void setValidSlots( int val );
	const QString *getSource() const;
	void setSource( QString* name );
	SymbolStatus status() const;
	void setStatus( SymbolStatus s );
	
	bool isSlotValid( MemoryLayout *ml = 0 );

private:
	QString text;
	int address;
	int validSlots;
	QList<unsigned char> validSegments;
	QString *source;
	SymbolStatus symbolStatus;
};


class SymbolTable
{
public:
	SymbolTable();
	~SymbolTable();

	void addAddressSymbol( AddressSymbol *symbol );
	void removeAddressSymbolAt( int index );

	AddressSymbol *findFirstAddressSymbol( int addr, MemoryLayout *ml = 0 );
	AddressSymbol *getCurrentAddressSymbol();
	AddressSymbol *findNextAddressSymbol( MemoryLayout *ml = 0 );

	int symbolFilesSize() const;
	const QString& symbolFile( int index ) const;

	bool readTNIASM0File( const QString& filename );
	void reloadFiles();
	void unloadFile( const QString& file, bool keepSymbols = false );

private:
	QList<AddressSymbol*> addressSymbols;
	QList<AddressSymbol*>::iterator currentSymbol;
	QList<QString*> symbolFiles;
};

#endif // _SYMBOLTABLE_H
