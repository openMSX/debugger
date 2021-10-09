#include "SymbolTable.h"
#include "DebuggerData.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegExp>
#include <QFileInfo>
#include <QXmlStreamWriter>
#include <QMap>

// class SymbolTable

SymbolTable::SymbolTable()
{
	connect(&fileWatcher, SIGNAL(fileChanged(const QString&)), this, SLOT(fileChanged(const QString&)));
}

SymbolTable::~SymbolTable()
{
	clear();
}

void SymbolTable::add(Symbol* symbol)
{
	symbols.append(symbol);
	symbol->table = this;
	mapSymbol(symbol);
}

void SymbolTable::removeAt(int index)
{
	Symbol* symbol = symbols.takeAt(index);
	unmapSymbol(symbol);
	delete symbol;
}

void SymbolTable::remove(Symbol* symbol)
{
	symbols.removeAll(symbol);
	unmapSymbol(symbol);
	delete symbol;
}

void SymbolTable::clear()
{
	addressSymbols.clear();
	valueSymbols.clear();
	qDeleteAll(symbols);
	symbols.clear();
}

int SymbolTable::size() const
{
	return symbols.size();
}

void SymbolTable::mapSymbol(Symbol* symbol)
{
	if (symbol->type() != Symbol::VALUE) {
		addressSymbols.insert(symbol->value(), symbol);
	}
	if (symbol->type() != Symbol::JUMPLABEL) {
		valueSymbols.insert(symbol->value(), symbol);
	}
}

void SymbolTable::unmapSymbol(Symbol* symbol)
{
	QMutableMapIterator<int, Symbol*> i(addressSymbols);
	while (i.hasNext()) {
		i.next();
		if (i.value() == symbol) i.remove();
	}
	QMutableHashIterator<int, Symbol*> j(valueSymbols);
	while (j.hasNext()) {
		j.next();
		if (j.value() == symbol) j.remove();
	}
}

void SymbolTable::symbolTypeChanged(Symbol* symbol)
{
	unmapSymbol(symbol);
	mapSymbol(symbol);
}

void SymbolTable::symbolValueChanged(Symbol* symbol)
{
	unmapSymbol(symbol);
	mapSymbol(symbol);
}

Symbol* SymbolTable::findFirstAddressSymbol(int addr, MemoryLayout* ml)
{
	for (currentAddress = addressSymbols.begin();
	     currentAddress != addressSymbols.end(); ++currentAddress) {
		if ((*currentAddress)->value() >= addr) {
			if ((*currentAddress)->isSlotValid(ml)) {
				return *currentAddress;
			}
		}
	}
	return nullptr;
}

Symbol* SymbolTable::getCurrentAddressSymbol()
{
	return currentAddress != addressSymbols.end() ? *currentAddress : nullptr;
}

Symbol* SymbolTable::findNextAddressSymbol(MemoryLayout* ml)
{
	for (++currentAddress; currentAddress != addressSymbols.end();
	     ++currentAddress) {
		if ((*currentAddress)->isSlotValid(ml)) {
			return *currentAddress;
		}
	}
	return nullptr;
}

Symbol* SymbolTable::getValueSymbol(int val, Symbol::Register reg, MemoryLayout* ml)
{
	for (auto it = valueSymbols.find(val); it != valueSymbols.end() && it.key() == val; ++it) {
		if ((it.value()->validRegisters() & reg) &&
		    it.value()->isSlotValid(ml)) {
			return it.value();
		}
	}
	return nullptr;
}

Symbol* SymbolTable::getAddressSymbol(int addr, MemoryLayout* ml)
{
	for (auto it = addressSymbols.find(addr); it != addressSymbols.end() && it.key() == addr; ++it) {
		if (it.value()->isSlotValid(ml)) {
			return it.value();
		}
	}
	return nullptr;
}

Symbol* SymbolTable::getAddressSymbol(const QString& label, bool case_sensitive)
{
	for (auto it = addressSymbols.begin(); it != addressSymbols.end(); ++it) {
		if (it.value()->text().compare(label, Qt::CaseSensitive)==0)
			return it.value();
		if (!case_sensitive && it.value()->text().compare(label, Qt::CaseInsensitive)==0)
			return it.value();
	}
	return nullptr;
}

QStringList SymbolTable::labelList(bool include_vars, const MemoryLayout* ml) const
{
	QStringList labels;
	for (auto it = addressSymbols.begin(); it != addressSymbols.end(); ++it) {
		if (it.value()->type() == Symbol::JUMPLABEL || (include_vars && it.value()->type() == Symbol::VARIABLELABEL)) {
			if (ml == nullptr || it.value()->isSlotValid(ml)) {
				labels << it.value()->text();
			}
		}
	}
	return labels;
}

int SymbolTable::symbolFilesSize() const
{
	return symbolFiles.size();
}

const QString& SymbolTable::symbolFile(int index) const
{
	return symbolFiles.at(index).fileName;
}

const QDateTime& SymbolTable::symbolFileRefresh(int index) const
{
	return symbolFiles.at(index).refreshTime;
}

bool SymbolTable::readFile(const QString& filename, FileType type)
{
	if (type == DETECT_FILE) {
		if (filename.toLower().endsWith(".map")) {
			// HiTech link map file
			type = LINKMAP_FILE;
		} else if (filename.toLower().endsWith(".sym")) {
			// auto detect which sym file
			QFile file(filename);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream in(&file);
				QString line = in.readLine();
				if (line[0] == ';') {
					type = ASMSX_FILE;
				} else if (line.contains("; last def. pass")) {
					type = TNIASM0_FILE;
				} else if (line.contains(": %equ ")) {
					type = TNIASM1_FILE;
				} else if (line.contains(": equ ", Qt::CaseInsensitive)) {
					type = SJASM_FILE;
				} else {
					// this is a blunt conclusion but I
					// don't know a way to detect this file
					// type
					type = HTC_FILE;
				}
			}
		} else {
			QString ext = filename.toLower();
			/* They are the same type of file. For some reason the Debian
			 * manpage uses the extension ".sys"
			 * pasmo doc -> pasmo [options] file.asm file.bin [file.symbol [file.publics] ]
			 * pasmo manpage in Debian -> pasmo [options]  file.asm file.bin [file.sys]
			*/
			if (ext.endsWith(".symbol") || ext.endsWith(".publics") || ext.endsWith(".sys")) {
				type = PASMO_FILE;
			}
		}
	}
	switch (type) {
	case TNIASM0_FILE:
		return readTNIASM0File(filename);
	case TNIASM1_FILE:
		return readTNIASM1File(filename);
	case SJASM_FILE:
		return readSJASMFile(filename);
	case ASMSX_FILE:
		return readASMSXFile(filename);
	case HTC_FILE:
		return readHTCFile(filename);
	case LINKMAP_FILE:
		return readLinkMapFile(filename);
	case PASMO_FILE:
		return readPASMOFile(filename);
	default:
		return false;
	}
}

void SymbolTable::appendFile(const QString& file, FileType type)
{
	SymbolFileRecord rec;
	rec.fileName = file;
	rec.fileType = type;
	rec.refreshTime = QDateTime::currentDateTime();
	symbolFiles.append(rec);
	fileWatcher.addPath(file);
}

// Universal value parsing routine. Accepts:
//  - 0123h, 1234h, 1234H  (hex)
//  - 0x1234               (hex)
//  - 1234                 (dec)
//  - 0123                 (oct)
// The string may (optionally) end with a '; comment' part (with or without
// whitespace around the ';' character).
static bool parseValue(const QString& str, int& result)
{
	QStringList l = str.split(";"); // ignore stuff after ';'
	QString s = l.at(0).trimmed();
	bool success;
	if (s.endsWith('h', Qt::CaseInsensitive)) {
		s.chop(1);
		result = s.toInt(&success, 16);
	} else {
		result = s.toInt(&success, 0); // any base (e.g. 0x..)
	}
	return success;
}

bool SymbolTable::readSymbolFile(
	const QString& filename, FileType type, const QString& equ)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	appendFile(filename, type);
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split(equ, Qt::KeepEmptyParts, Qt::CaseInsensitive);
		if (l.size() != 2) continue;
		int value;
		if (!parseValue(l.at(1), value)) continue;
		auto* sym = new Symbol(l.at(0), value);
		sym->setSource(&symbolFiles.back().fileName);
		add(sym);
	}
	return true;
}
bool SymbolTable::readTNIASM0File(const QString& filename)
{
	return readSymbolFile(filename, TNIASM0_FILE, ": equ ");
}
bool SymbolTable::readTNIASM1File(const QString& filename)
{
	return readSymbolFile(filename, TNIASM1_FILE, ": %equ ");
}
bool SymbolTable::readSJASMFile(const QString& filename)
{
	return readSymbolFile(filename, SJASM_FILE, ": equ ");
}

bool SymbolTable::readASMSXFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	appendFile(filename, ASMSX_FILE);
	QTextStream in(&file);
	int filePart = 0;
	while (!in.atEnd()) {
		QString line = in.readLine();
		if (line[0] == ';') {
			if (line.startsWith("; global and local")) {
				filePart = 1;
			} else if (line.startsWith("; other")) {
				filePart = 2;
			}
		} else {
			if ((line[0] == '$') || (line[4] == 'h') ||
			    (line[5] == 'h') || (line[8] == 'h')) {
				if (filePart == 1) {
					QStringList l = line.split(" ");
					Symbol* sym;
					if (line[0] == '$') {
						sym = new Symbol(l.at(1).trimmed(), l.at(0).right(4).toInt(nullptr, 16));
					} else if ((line[4] == 'h') || (line[5] == 'h')) {
						sym = new Symbol(l.at(1).trimmed(), l.at(0).mid(l.at(0).indexOf('h') - 4, 4).toInt(nullptr, 16));
					} else {
						QStringList n = l.at(0).split(":"); // n.at(0) = MegaROM page
						sym = new Symbol(l.at(1).trimmed(), n.at(1).left(4).toInt(nullptr, 16));
					}
					sym->setSource(&symbolFiles.back().fileName);
					add(sym);
				} else if (filePart == 2) {
					//
				}
			}
		}
	}
	return true;
}

bool SymbolTable::readPASMOFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}
	appendFile(filename, PASMO_FILE);
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line;
		QStringList l;
		Symbol* sym;
		line = in.readLine();
		l = line.split(QRegExp("(\t+)|( +)"));
		if (l.size() == 3) {
			sym = new Symbol(l.at(0), l.at(2).left(5).toInt(nullptr, 16));
			sym->setSource(&symbolFiles.back().fileName);
			add(sym);
		}
	}
	return true;
}

bool SymbolTable::readHTCFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	appendFile(filename, HTC_FILE);
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split(' ');
		if (l.size() != 3) continue;
		int value;
		if (!parseValue("0x" + l.at(1), value)) continue;
		auto* sym = new Symbol(l.at(0), value);
		sym->setSource(&symbolFiles.back().fileName);
		add(sym);
	}
	return true;
}

bool SymbolTable::readLinkMapFile(const QString& filename)
{
	const QString magic("Machine type");
	const QString tableStart("*\tSymbol Table");

	QRegExp rx(" [0-9A-Fa-f]{4}  (?![ 0-9])");
	QRegExp rp("^([^ ]+) +[^ ]* +([0-9A-Fa-f]{4})  $");

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}
	appendFile(filename, LINKMAP_FILE);

	QTextStream in(&file);
	if (in.atEnd()) return false;
	if (!in.readLine().startsWith(magic)) return false;
	while (true) {
		if (in.atEnd()) return false;
		if (in.readLine().startsWith(tableStart)) break;
	}

	while (!in.atEnd()) {
		QString line = in.readLine();
		Q_ASSERT(!line.endsWith("\r"));
		if (line.isEmpty()) continue;

		line += "  ";
		int len = line.length();
		int l;
		int pos = 0;
		bool ok = false;
		// HiTech uses multiple columns of non-fixed width and
		// a column for psect may be blank so the address may in
		// the first or second match.
		for (int tries = 0; (tries < 2) && !ok; ++tries) {
			pos = rx.indexIn(line, pos);
			l = pos + rx.matchedLength();
			if ((l > 0) && (len % l) == 0) {
				ok = true;
				for (int posn = pos + l; (posn < len) && ok; posn += l) {
					ok = (posn == rx.indexIn(line, posn));
				}
			}
			pos = l - 1;
		}
		if (!ok) continue;

		for (pos = 0; pos < len; pos += l) {
			QString part = line.mid(pos, l);
			if (rp.indexIn(part) == 0) {
				QStringList l = rp.capturedTexts();
				auto* sym = new Symbol(l.at(1), l.last().toInt(nullptr, 16));
				sym->setSource(&symbolFiles.back().fileName);
				add(sym);
			}
		}
	}
	return true;
}

void SymbolTable::fileChanged(const QString & /*path*/)
{
	emit symbolFileChanged();
}

void SymbolTable::reloadFiles()
{
	for (int i = 0; i < symbolFiles.size(); ++i) {
		// check if file is newer
		QFileInfo fi = QFileInfo(symbolFiles[i].fileName);
		if (fi.lastModified() <= symbolFiles[i].refreshTime) continue;

		// file info
		QString name = symbolFiles[i].fileName;
		FileType type = symbolFiles[i].fileType;
		// symbol file is newer
		QMap<QString, Symbol> symCopy;
		// copy all symbols originating from this file
		QMutableListIterator<Symbol*> si(symbols);
		while (si.hasNext()) {
			si.next();
			if (si.value()->source() == &symbolFiles[i].fileName) {
				symCopy.insert(si.value()->text(), Symbol(*si.value()));
			}
		}
		// remove existing file
		unloadFile(name);
		// read the new file
		readFile(name, type);
		// find old symbols in newly loaded file
		QMutableListIterator<Symbol*> ni(symbols);
		QString* newFile = &symbolFiles.back().fileName;
		while (ni.hasNext()) {
			ni.next();
			if (ni.value()->source() != newFile) continue;

			// find symbol in old list
			auto sit = symCopy.find(ni.value()->text());
			if (sit == symCopy.end()) continue;

			// symbol existed before, copy settings
			ni.value()->setValidSlots(sit->validSlots());
			ni.value()->setValidRegisters(sit->validRegisters());
			ni.value()->setType(sit->type());
			if (sit->status() == Symbol::LOST) {
				ni.value()->setStatus(Symbol::ACTIVE);
			} else {
				ni.value()->setStatus(sit->status());
			}
			symCopy.erase(sit);
		}
		// all symbols left in map are lost
		for (auto sit = symCopy.begin(); sit != symCopy.end(); ++sit) {
			auto* sym = new Symbol(sit.value());
			sym->setStatus(Symbol::LOST);
			sym->setSource(newFile);
			add(sym);
		}
	}
}

void SymbolTable::unloadFile(const QString& file, bool keepSymbols)
{
	int index = -1;
	for (int i = 0; i < symbolFiles.size(); ++i) {
		if (symbolFiles[i].fileName == file) {
			index = i;
			break;
		}
	}
	if (index >= 0) {
		QString* name = &symbolFiles[index].fileName;

		if (!keepSymbols) {
			// remove symbols from address map
			QMutableMapIterator<int, Symbol*> mi(addressSymbols);
			while (mi.hasNext()) {
				mi.next();
				if (mi.value()->source() == name) mi.remove();
			}
			// remove symbols from value hash
			QMutableHashIterator<int, Symbol*> hi(valueSymbols);
			while (hi.hasNext()) {
				hi.next();
				if (hi.value()->source() == name) hi.remove();
			}
		}
		// remove symbols from value hash
		QMutableListIterator<Symbol*> i(symbols);
		while (i.hasNext()) {
			i.next();
			Symbol* sym = i.value();
			if (sym->source() == name) {
				if (keepSymbols) {
					sym->setSource(nullptr);
				} else {
					i.remove();
					delete sym;
				}
			}
		}
		// remove record
		fileWatcher.removePath(symbolFiles[index].fileName);
		symbolFiles.removeAt(index);
	}
}

/*
 * Session loading/saving
 */
void SymbolTable::saveSymbols(QXmlStreamWriter& xml)
{
	// write files
	QMap<const QString*, int> fileIds;
	for (int i = 0; i < symbolFiles.size(); ++i) {
		// add id mapping
		fileIds[&symbolFiles[i].fileName] = i;
		// write element
		xml.writeStartElement("SymbolFile");
		switch (symbolFiles[i].fileType) {
		case TNIASM0_FILE:
			xml.writeAttribute("type","tniasm0");
			break;
		case TNIASM1_FILE:
			xml.writeAttribute("type","tniasm1");
			break;
		case ASMSX_FILE:
			xml.writeAttribute("type","asmsx");
			break;
		case LINKMAP_FILE:
			xml.writeAttribute("type","linkmap");
			break;
		default:
			break;
		}
		xml.writeAttribute("refreshTime",
		                   QString::number(symbolFiles[i].refreshTime.toTime_t()));
		xml.writeCharacters(symbolFiles[i].fileName);
		xml.writeEndElement();
	}
	// write symbols
	for (auto* sym : symbols) {
	     	xml.writeStartElement("Symbol");
		// status
		if (sym->status() == Symbol::HIDDEN) {
			xml.writeAttribute("status", "hidden");
		} else if (sym->status() == Symbol::LOST) {
			xml.writeAttribute("status", "lost");
		}
		// type
		if (sym->type() == Symbol::JUMPLABEL) {
			xml.writeTextElement("type", "jump");
		} else if (sym->type() == Symbol::VARIABLELABEL) {
			xml.writeTextElement("type", "variable");
		} else {
			xml.writeTextElement("type", "value");
		}
		// text, value, slots and registers
		xml.writeTextElement("name", sym->text());
		xml.writeTextElement("value", QString::number(sym->value()));
		xml.writeTextElement("validSlots", QString::number(sym->validSlots()));
		xml.writeTextElement("validRegisters", QString::number(sym->validRegisters()));
		// write source filename
		if (sym->source()) {
			xml.writeTextElement("source", QString::number(fileIds[sym->source()]));
		}
		// complete
		xml.writeEndElement();
	}
}

void SymbolTable::loadSymbols(QXmlStreamReader& xml)
{
	Symbol* sym;
	while (!xml.atEnd()) {
		xml.readNext();
		// exit if closing of main tag
		if (xml.isEndElement() && xml.name() == "Symbols") break;

		// begin tag
		if (xml.isStartElement()) {
			if (xml.name() == "SymbolFile") {

				// read attributes and text
				QString ftype = xml.attributes().value("type").toString().toLower();
				QString rtime = xml.attributes().value("refreshTime").toString();
				QString fname = xml.readElementText();
				// check type
				FileType type = TNIASM0_FILE;
				if (ftype == "tniasm1") {
					type = TNIASM1_FILE;
				} else if (ftype == "asmsx") {
					type = ASMSX_FILE;
				} else if (ftype == "linkmap") {
					type = LINKMAP_FILE;
				}
				// append file
				appendFile(fname, type);
				// change time
				symbolFiles.back().refreshTime.setTime_t(rtime.toUInt());

			} else if (xml.name() == "Symbol") {
				// add empty symbol
				sym = new Symbol("", 0);
				add(sym);
				// get status attribute
				QString stat = xml.attributes().value("status").toString().toLower();
				if (stat == "hidden") {
					sym->setStatus(Symbol::HIDDEN);
				} else if (stat == "lost") {
					sym->setStatus(Symbol::LOST);
				}

			} else if (xml.name() == "type") {
				// read symbol type element
				QString type = xml.readElementText().trimmed().toLower();
				if (type == "jump") {
					sym->setType(Symbol::JUMPLABEL);
				} else if (type == "variable") {
					sym->setType(Symbol::VARIABLELABEL);
				} else if (type == "value") {
					sym->setType(Symbol::VALUE);
				}

			} else if (xml.name() == "name") {
				// read symbol name
				sym->setText(xml.readElementText());

			} else if (xml.name() == "value") {
				// read symbol value
				sym->setValue(xml.readElementText().toInt());

			} else if (xml.name() == "validSlots") {
				// read numeric valid slot mask
				sym->setValidSlots(xml.readElementText().toInt());

			} else if (xml.name() == "validRegisters") {
				// read numeric valid registers mask
				sym->setValidRegisters(xml.readElementText().toInt());

			} else if (xml.name() == "source") {
				// read source file id
				int id = xml.readElementText().toInt();
				if (id >= 0 && id < symbolFiles.size()) {
					sym->setSource(&symbolFiles[id].fileName);
				}
			}
		}
	}
}


// class Symbol

Symbol::Symbol(const QString& str, int addr, int val)
	: symText(str), symValue(addr), symSlots(val)
{
	symStatus = ACTIVE;
	symType = JUMPLABEL;
	symSource = nullptr;
	if (addr & 0xFF00) {
		symRegisters = REG_ALL16;
	} else {
		symRegisters = REG_ALL;
	}
	table = nullptr;
}

Symbol::Symbol(const Symbol& symbol)
{
	table = nullptr;
	symStatus    = symbol.symStatus;
	symText      = symbol.symText;
	symValue     = symbol.symValue;
	symSlots     = symbol.symSlots;
	symSegments  = symbol.symSegments;
	symRegisters = symbol.symRegisters;
	symSource    = symbol.symSource;
	symType      = symbol.symType;
}

const QString& Symbol::text() const
{
	return symText;
}

void Symbol::setText(const QString& str)
{
	symText = str;
}

int Symbol::value() const
{
	return symValue;
}

void Symbol::setValue(int addr)
{
	if (addr == symValue) return;

	symValue = addr;
	if (table) table->symbolValueChanged(this);
}

int Symbol::validSlots() const
{
	return symSlots;
}

void Symbol::setValidSlots(int val)
{
	symSlots = val & 0xFFFF;
}

int Symbol::validRegisters() const
{
	return symRegisters;
}

void Symbol::setValidRegisters(int regs)
{
	symRegisters = regs;
	if (symValue & 0xFF00) {
		symRegisters &= REG_ALL16;
	}
}

const QString* Symbol::source() const
{
	return symSource;
}

void Symbol::setSource(const QString* name)
{
	symSource = name;
}

Symbol::SymbolStatus Symbol::status() const
{
	return symStatus;
}

void Symbol::setStatus(SymbolStatus s)
{
	symStatus = s;
}

Symbol::SymbolType Symbol::type() const
{
	return symType;
}

void Symbol::setType(SymbolType t)
{
	if (symType == t) return;

	symType = t;
	if (table) table->symbolTypeChanged(this);
}

bool Symbol::isSlotValid(const MemoryLayout* ml) const
{
	if (!ml) return true;
	int page = symValue >> 14;
	int ps = ml->primarySlot[page] & 3;
	int ss = 0;
	if (ml->isSubslotted[page]) ss = ml->secondarySlot[page] & 3;
	if (symSlots & (1 << (4 * ps + ss))) {
		if (symSegments.empty()) return true;
		for (const auto& seg : symSegments) {
			if (ml->mapperSegment[page] == seg) {
				return true;
			}
		}
	}
	return false;
}
