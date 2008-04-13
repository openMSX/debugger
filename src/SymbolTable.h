// $Id$
#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <QString>
#include <QList>
#include <QMultiMap>
#include <QMultiHash>
#include <QDateTime>


struct MemoryLayout;

class SymbolTable;

class Symbol
{
public:
	Symbol( const QString& str, int addr, int val = 0xFFFF );
	Symbol( const Symbol& symbol );
	
	friend class SymbolTable;

	// ACTIVE status is for regular symbols. HIDDEN is for symbols
	// that are in the list but not shown anywhere. LOST is a special
	// status for symbols that were once loaded from a symbol file, but
	// weren't found later. These aren't deleted immediately because
	// the possible custom settings would be lost even if the reload
	// was of a bad file (after a failed assembler run for instance).
	enum SymbolStatus { ACTIVE, HIDDEN, LOST };
	enum SymbolType { JUMPLABEL, VARIABLELABEL, VALUE };
	enum Register { REG_A = 1, REG_B = 2, REG_C = 4, REG_D = 8, REG_E = 16,
	                REG_H = 32, REG_L = 64, REG_BC = 128, REG_DE = 256,
	                REG_HL = 512, REG_IX = 1024, REG_IY = 2048, REG_IXL = 4096,
	                REG_IXH = 8192, REG_IYL = 16384, REG_IYH = 32768,
	                REG_OFFSET = 65536, REG_I = 131072, 
	                REG_ALL8 = 1+2+4+8+16+32+64+4096+8192+16384+32768+65536+131072,
	                REG_ALL16 = 128+256+512+1024+2048,
	                REG_ALL = 0x3FFFF };
	
	const QString& text() const;
	void setText( const QString& str );
	int value() const;
	void setValue( int addr );
	int validSlots() const;
	void setValidSlots( int val );
	int validRegisters() const;
	void setValidRegisters( int regs );
	const QString *source() const;
	void setSource( QString* name );
	SymbolStatus status() const;
	void setStatus( SymbolStatus s );
	SymbolType type() const;
	void setType( SymbolType t );
	
	bool isSlotValid( MemoryLayout *ml = 0 );

private:
	SymbolTable *table;

	QString symText;
	int symValue;
	int symSlots;
	QList<unsigned char> symSegments;
	int symRegisters;
	QString *symSource;
	SymbolStatus symStatus;
	SymbolType symType;
};


class SymbolTable
{
public:
	SymbolTable();
	~SymbolTable();

	void add( Symbol *symbol );
	void removeAt( int index );
	void remove( Symbol *symbol );

	/* Symbol access functions */
	Symbol *findFirstAddressSymbol( int addr, MemoryLayout *ml = 0 );
	Symbol *getCurrentAddressSymbol();
	Symbol *findNextAddressSymbol( MemoryLayout *ml = 0 );
	Symbol *getValueSymbol( int val, Symbol::Register reg, MemoryLayout *ml = 0 );
	Symbol *getAddressSymbol( int val, MemoryLayout *ml = 0 );

	void symbolTypeChanged( Symbol *symbol );
	void symbolValueChanged( Symbol *symbol );

	int symbolFilesSize() const;
	const QString& symbolFile( int index ) const;
	const QDateTime& symbolFileRefresh( int index ) const;

	enum FileType { DETECT_FILE, TNIASM_FILE, ASMSX_FILE, LINKMAP_FILE };

	bool readFile( const QString& filename, FileType type = DETECT_FILE );
	void reloadFiles();
	void unloadFile( const QString& file, bool keepSymbols = false );

private:
	QList<Symbol*> symbols;
	QMultiMap<int, Symbol*> addressSymbols;
	QMultiHash<int, Symbol*> valueSymbols;
	QMultiMap<int, Symbol*>::iterator currentAddress;
	
	typedef struct {
		QString fileName;
		QDateTime refreshTime;
		FileType fileType;
	} SymbolFileRecord;
	QList<SymbolFileRecord> symbolFiles;
	
	void appendFile( const QString& file, FileType type );
	bool readTNIASM0File( const QString& filename );
	bool readASMSXFile( const QString& filename );
	bool readLinkMapFile( const QString& filename );

	void mapSymbol( Symbol *symbol );
	void unmapSymbol( Symbol *symbol );
};


#endif // _SYMBOLTABLE_H
