#include "SymbolTable.h"
#include "Settings.h"
#include "DebuggerData.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegExp>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMap>
#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>

// class SymbolTable

SymbolTable::SymbolTable()
{
	connect(&fileWatcher, &QFileSystemWatcher::fileChanged,
	        this, &SymbolTable::fileChanged);
}

Symbol* SymbolTable::add(std::unique_ptr<Symbol> symbol)
{
	auto* p = symbol.get();
	symbols.push_back(std::move(symbol));
	p->table = this;
	mapSymbol(p);
	return p;
}

std::unique_ptr<Symbol> SymbolTable::removeAt(size_t index)
{
	assert(index < symbols.size());
	auto symbol = std::move(symbols[index]);
	symbols.erase(symbols.begin() + index); // TODO optimize: swap with back(), then pop_back()
	unmapSymbol(symbol.get());
	return symbol;
}

std::unique_ptr<Symbol> SymbolTable::remove(Symbol* symbol)
{
	auto it = std::find_if(symbols.begin(), symbols.end(),
	                       [&](auto& e) { return e.get() == symbol; });
	if (it == symbols.end()) return {};

	return removeAt(std::distance(symbols.begin(), it));
}

void SymbolTable::clear()
{
	addressSymbols.clear();
	valueSymbols.clear();
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
	QString fname = filename.toLower();

	if (type == DETECT_FILE) {
		if (fname.endsWith(".omds")) {
			// OpenMSX Debugger session file
			type = OMDS_FILE;
		} else if (fname.endsWith(".noi")) {
			// NoICE command file
			type = NOICE_FILE;
		} else if (fname.endsWith(".map")) {
			// HiTech link map file
			type = LINKMAP_FILE;
		} else if (fname.endsWith(".sym")) {
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
		} else if (fname.endsWith(".symbol") || fname.endsWith(".publics") || fname.endsWith(".sys")) {
			/* They are the same type of file. For some reason the Debian
			 * manpage uses the extension ".sys"
			 * pasmo doc -> pasmo [options] file.asm file.bin [file.symbol [file.publics] ]
			 * pasmo manpage in Debian -> pasmo [options]  file.asm file.bin [file.sys]
			*/
			type = PASMO_FILE;
		}
	}
	switch (type) {
	case OMDS_FILE:
		return readOMDSFile(filename);
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
	case NOICE_FILE:
		return readNoICEFile(filename);
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
static std::optional<int> parseValue(const QString& str)
{
	QStringList l = str.split(";"); // ignore stuff after ';'
	QString s = l.at(0).trimmed();
	bool success;
	int result;
	if (s.endsWith('h', Qt::CaseInsensitive)) {
		s.chop(1);
		result = s.toInt(&success, 16);
	} else {
		result = s.toInt(&success, 0); // any base (e.g. 0x..)
	}
	if (success) return result;
	return {};
}

bool SymbolTable::readSymbolFile(
	const QString& filename, FileType type, const QString& equ)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	appendFile(filename, type);
	const auto* source = &symbolFiles.back().fileName;

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split(equ, Qt::SplitBehaviorFlags::KeepEmptyParts, Qt::CaseInsensitive);
		if (l.size() != 2) continue;
		if (auto value = parseValue(l.at(1))) {
			add(std::make_unique<Symbol>(l.at(0), *value, source));
		}
	}
	return true;
}
bool SymbolTable::readOMDSFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return false;
	}

	QXmlStreamReader ses;
	ses.setDevice(&file);
	loadSymbols(ses);
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
	const auto* source = &symbolFiles.back().fileName;

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
					std::unique_ptr<Symbol> sym;
					if (line[0] == '$') {
						add(std::make_unique<Symbol>(l.at(1).trimmed(), l.at(0).right(4).toInt(nullptr, 16), source));
					} else if ((line[4] == 'h') || (line[5] == 'h')) {
						add(std::make_unique<Symbol>(l.at(1).trimmed(), l.at(0).mid(l.at(0).indexOf('h') - 4, 4).toInt(nullptr, 16), source));
					} else {
						QStringList n = l.at(0).split(":"); // n.at(0) = MegaROM page
						add(std::make_unique<Symbol>(l.at(1).trimmed(), n.at(1).left(4).toInt(nullptr, 16), source));
					}
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
	const auto* source = &symbolFiles.back().fileName;

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line;
		QStringList l;
		line = in.readLine();
		l = line.split(QRegExp("(\t+)|( +)"));
		if (l.size() == 3) {
			add(std::make_unique<Symbol>(l.at(0), l.at(2).left(5).toInt(nullptr, 16), source));
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
	const auto* source = &symbolFiles.back().fileName;

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split(' ');
		if (l.size() != 3) continue;
		if (auto value = parseValue("0x" + l.at(1))) {
			add(std::make_unique<Symbol>(l.at(0), *value, source));
		}
	}
	return true;
}

bool SymbolTable::readNoICEFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	appendFile(filename, NOICE_FILE);
	const auto* source = &symbolFiles.back().fileName;

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList l = line.split(" ", Qt::SplitBehaviorFlags::KeepEmptyParts, Qt::CaseInsensitive);
		if (l.size() != 3) continue;
		if (l.at(0).toLower() != "def") continue;
		if (auto value = parseValue(l.at(2))) {
			add(std::make_unique<Symbol>(l.at(1), *value, source));
		}
	}
	return true;
}

bool SymbolTable::readLinkMapFile(const QString& filename)
{
	const QString magic("Machine type");
	const QString tableStart("Symbol Table");

	QRegExp rx(" [0-9A-Fa-f]{4}  (?![ 0-9])");
	QRegExp rp("^([^ ]+) +[^ ]* +([0-9A-Fa-f]{4})  $");

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}
	appendFile(filename, LINKMAP_FILE);
	const auto* source = &symbolFiles.back().fileName;

	QTextStream in(&file);
	if (in.atEnd()) return false;
	while (true) {
		if (in.atEnd()) return false;
		if (in.readLine().startsWith(magic)) break;
	}
	while (true) {
		if (in.atEnd()) return false;
		if (in.readLine().contains(tableStart)) break;
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
				add(std::make_unique<Symbol>(l.at(1), l.last().toInt(nullptr, 16), source));
			}
		}
	}
	return true;
}

void SymbolTable::fileChanged(const QString& path)
{
	emit symbolFileChanged();
	if (QFile::exists(path)) {
		fileWatcher.addPath(path);
	}
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
		for (const auto& s : symbols) {
			if (s->source() == &symbolFiles[i].fileName) {
				symCopy.insert(s->text(), Symbol(*s));
			}
		}
		// remove existing file
		unloadFile(name);
		// read the new file
		readFile(name, type);
		// find old symbols in newly loaded file
		QString* newFile = &symbolFiles.back().fileName;
		for (auto& s : symbols) {
			if (s->source() != newFile) continue;

			// find symbol in old list
			auto sit = symCopy.find(s->text());
			if (sit == symCopy.end()) continue;

			// symbol existed before, copy settings
			s->setValidSlots(sit->validSlots());
			s->setValidRegisters(sit->validRegisters());
			s->setType(sit->type());
			s->setStatus((sit->status() == Symbol::LOST) ? Symbol::ACTIVE : sit->status());
			symCopy.erase(sit);
		}
		if (Settings::get().preserveLostSymbols()) {
			// all symbols left in map are lost
			for (auto sit = symCopy.begin(); sit != symCopy.end(); ++sit) {
				auto* sym = add(std::make_unique<Symbol>(sit.value()));
				sym->setStatus(Symbol::LOST);
				sym->setSource(newFile);
			}
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
		symbols.erase(std::remove_if(symbols.begin(), symbols.end(),
			[&](auto& sym) {
				if (sym->source() != name) {
					return false; // keep symbols from different source
				}
				if (!keepSymbols) {
					return true; // remove
				}
				// keep but clear source
				sym->setSource(nullptr);
				return false; // keep
			}), symbols.end());
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
	for (const auto& sym : symbols) {
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
	Symbol* sym = nullptr;
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
				sym = add(std::make_unique<Symbol>("", 0));
				// get status attribute
				QString stat = xml.attributes().value("status").toString().toLower();
				if (stat == "hidden") {
					sym->setStatus(Symbol::HIDDEN);
				} else if (stat == "lost") {
					sym->setStatus(Symbol::LOST);
				}

			} else if (sym && xml.name() == "type") {
				// read symbol type element
				QString type = xml.readElementText().trimmed().toLower();
				if (type == "jump") {
					sym->setType(Symbol::JUMPLABEL);
				} else if (type == "variable") {
					sym->setType(Symbol::VARIABLELABEL);
				} else if (type == "value") {
					sym->setType(Symbol::VALUE);
				}

			} else if (sym && xml.name() == "name") {
				// read symbol name
				sym->setText(xml.readElementText());

			} else if (sym && xml.name() == "value") {
				// read symbol value
				sym->setValue(xml.readElementText().toInt());

			} else if (sym && xml.name() == "validSlots") {
				// read numeric valid slot mask
				sym->setValidSlots(xml.readElementText().toInt());

			} else if (sym && xml.name() == "validRegisters") {
				// read numeric valid registers mask
				sym->setValidRegisters(xml.readElementText().toInt());

			} else if (sym && xml.name() == "source") {
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

Symbol::Symbol(QString str, int addr, const QString* source)
	: symText(std::move(str)), symValue(addr), symSource(source)
{
	symRegisters = (addr & 0xFF00) ? REG_ALL16 : REG_ALL;
}

Symbol::Symbol(const Symbol& symbol)
{
	table = nullptr;
	symStatus    = symbol.symStatus;
	symText      = symbol.symText;
	symValue     = symbol.symValue;
	symSlots     = symbol.symSlots;
	//symSegments  = symbol.symSegments;
	symRegisters = symbol.symRegisters;
	symSource    = symbol.symSource;
	symType      = symbol.symType;
}

void Symbol::setValue(int addr)
{
	if (addr == symValue) return;

	symValue = addr;
	if (table) table->symbolValueChanged(this);
}

void Symbol::setValidRegisters(int regs)
{
	symRegisters = regs;
	if (symValue & 0xFF00) {
		symRegisters &= REG_ALL16;
	}
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
		return true;
		//if (symSegments.empty()) return true;
		//for (const auto& seg : symSegments) {
		//	if (ml->mapperSegment[page] == seg) {
		//		return true;
		//	}
		//}
	}
	return false;
}
