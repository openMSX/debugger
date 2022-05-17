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
#include <cstdint>
#include <memory>
#include <vector>

struct MemoryLayout;
class SymbolTable;

class Symbol
{
public:
	Symbol(QString str, int addr, const QString* source = nullptr);
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
	enum Register { REG_A = 1 << 0, REG_B = 1 << 1, REG_C = 1 << 2, REG_D = 1 << 3, REG_E = 1 << 4,
	                REG_H = 1 << 5, REG_L = 1 << 6, REG_BC = 1 << 7, REG_DE = 1 << 8,
	                REG_HL = 1 << 9, REG_IX = 1 << 10, REG_IY = 1 << 11, REG_IXL = 1 << 12,
	                REG_IXH = 1 << 13, REG_IYL = 1 << 15, REG_IYH = 1 << 15,
	                REG_OFFSET = 1 << 16, REG_I = 1 << 17,
			REG_ALL8 = REG_A | REG_B | REG_C | REG_D | REG_E | REG_H | REG_L
			         | REG_IXL | REG_IXH | REG_IYL | REG_IYH
			         | REG_OFFSET | REG_I,
	                REG_ALL16 = REG_BC | REG_DE | REG_HL | REG_IX | REG_IY,
	                REG_ALL = REG_ALL8 | REG_ALL16 };

	[[nodiscard]] const QString& text() const { return symText; }
	void setText(const QString& str) { symText = str; }
	[[nodiscard]] int value() const { return symValue; }
	void setValue(int addr);
	[[nodiscard]] uint16_t validSlots() const { return symSlots; }
	void setValidSlots(uint16_t val) { symSlots = val; }
	[[nodiscard]] int validRegisters() const { return symRegisters; }
	void setValidRegisters(int regs);
	[[nodiscard]] const QString* source() const { return symSource; }
	void setSource(const QString* name) { symSource = name; }
	[[nodiscard]] SymbolStatus status() const { return symStatus; }
	void setStatus(SymbolStatus s) { symStatus = s; }
	[[nodiscard]] SymbolType type() const { return symType; }
	void setType(SymbolType t);

	bool isSlotValid(const MemoryLayout* ml = nullptr) const;

private:
	SymbolTable* table = nullptr;

	QString symText;
	int symValue;
	uint16_t symSlots = 0xffff;
	//QList<uint8_t> symSegments;
	int symRegisters;
	const QString* symSource = nullptr;
	SymbolStatus symStatus = ACTIVE;
	SymbolType symType = JUMPLABEL;

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

	Symbol* add(std::unique_ptr<Symbol> symbol);
	std::unique_ptr<Symbol> removeAt(size_t index);
	std::unique_ptr<Symbol> remove(Symbol *symbol);
	void clear();
	[[nodiscard]] int size() const;

	// xml session file functions
	void saveSymbols(QXmlStreamWriter& xml);
	void loadSymbols(QXmlStreamReader& xml);

	// Symbol access functions
	[[nodiscard]] Symbol* findFirstAddressSymbol(int addr, MemoryLayout* ml = nullptr);
	[[nodiscard]] Symbol* getCurrentAddressSymbol();
	[[nodiscard]] Symbol* findNextAddressSymbol(MemoryLayout* ml = nullptr);
	[[nodiscard]] Symbol* getValueSymbol(int val, Symbol::Register reg, MemoryLayout* ml = nullptr);
	[[nodiscard]] Symbol* getAddressSymbol(int val, MemoryLayout* ml = nullptr);
	[[nodiscard]] Symbol* getAddressSymbol(const QString& label, bool case_sensitive = false);

	[[nodiscard]] QStringList labelList(bool include_vars = false, const MemoryLayout* ml = nullptr) const;

	void symbolTypeChanged(Symbol* symbol);
	void symbolValueChanged(Symbol* symbol);

	[[nodiscard]] int symbolFilesSize() const;
	[[nodiscard]] const QString& symbolFile(int index) const;
	[[nodiscard]] const QDateTime& symbolFileRefresh(int index) const;

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
	std::vector<std::unique_ptr<Symbol>> symbols;
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
