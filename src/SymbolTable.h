#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <QString>
#include <QList>
#include <QMultiMap>
#include <QMultiHash>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFileSystemWatcher>

struct MemoryLayout;
class SymbolTable;

class Symbol
{
public:
	Symbol(QString str, int addr, int val = 0xFFFF);
	Symbol(const Symbol& symbol);
	Symbol& operator=(const Symbol&) = default;

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
	void setText(const QString& str);
	int value() const;
	void setValue(int addr);
	int validSlots() const;
	void setValidSlots(int val);
	int validRegisters() const;
	void setValidRegisters(int regs);
	const QString* source() const;
	void setSource(const QString* name);
	SymbolStatus status() const;
	void setStatus(SymbolStatus s);
	SymbolType type() const;
	void setType(SymbolType t);

	bool isSlotValid(const MemoryLayout* ml = nullptr) const;

private:
	SymbolTable* table;

	QString symText;
	int symValue;
	int symSlots;
	QList<uint8_t> symSegments;
	int symRegisters;
	const QString* symSource;
	SymbolStatus symStatus;
	SymbolType symType;

	friend class SymbolTable;
};


class SymbolTable : public QObject
{
	Q_OBJECT
public:
	enum FileType {
		DETECT_FILE,
		TNIASM0_FILE,
		TNIASM1_FILE,
		SJASM_FILE,
		ASMSX_FILE,
		LINKMAP_FILE,
		HTC_FILE,
		NOICE_FILE,
		PASMO_FILE
	};

	SymbolTable();
	~SymbolTable() override;

	void add(Symbol* symbol);
	void removeAt(int index);
	void remove(Symbol *symbol);
	void clear();
	int size() const;

	/* xml session file functions */
	void saveSymbols(QXmlStreamWriter& xml);
	void loadSymbols(QXmlStreamReader& xml);

	/* Symbol access functions */
	Symbol* findFirstAddressSymbol(int addr, MemoryLayout* ml = nullptr);
	Symbol* getCurrentAddressSymbol();
	Symbol* findNextAddressSymbol(MemoryLayout* ml = nullptr);
	Symbol* getValueSymbol(int val, Symbol::Register reg, MemoryLayout* ml = nullptr);
	Symbol* getAddressSymbol(int val, MemoryLayout* ml = nullptr);
	Symbol* getAddressSymbol(const QString& label, bool case_sensitive = false);

	QStringList labelList(bool include_vars = false, const MemoryLayout* ml = nullptr) const;

	void symbolTypeChanged(Symbol* symbol);
	void symbolValueChanged(Symbol* symbol);

	int symbolFilesSize() const;
	const QString& symbolFile(int index) const;
	const QDateTime& symbolFileRefresh(int index) const;

	bool readFile(const QString& filename, FileType type = DETECT_FILE);
	void reloadFiles();
	void unloadFile(const QString& file, bool keepSymbols = false);

signals:
	void symbolFileChanged();

private:
	void appendFile(const QString& file, FileType type);
	bool readSymbolFile(
		const QString& filename, FileType type, const QString& equ);
	bool readTNIASM0File(const QString& filename);
	bool readTNIASM1File(const QString& filename);
	bool readASMSXFile(const QString& filename);
	bool readSJASMFile(const QString& filename);
	bool readHTCFile(const QString& filename);
	bool readNoICEFile(const QString& filename);
	bool readLinkMapFile(const QString& filename);
	bool readPASMOFile(const QString& filename);

	void mapSymbol(Symbol* symbol);
	void unmapSymbol(Symbol* symbol);

	void fileChanged(const QString & path);

private:
	QList<Symbol*> symbols;
	QMultiMap<int, Symbol*> addressSymbols;
	QMultiHash<int, Symbol*> valueSymbols;
	QMultiMap<int, Symbol*>::iterator currentAddress;

	struct SymbolFileRecord {
		QString fileName;
		QDateTime refreshTime;
		FileType fileType;
	};
	QList<SymbolFileRecord> symbolFiles;
	QFileSystemWatcher fileWatcher;
};

#endif // SYMBOLTABLE_H
