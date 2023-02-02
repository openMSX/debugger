#include "Dasm.h"
#include "DasmTables.h"
#include "SymbolTable.h"
#include <sstream>
#include <iomanip>

static char sign(uint8_t a)
{
	return (a & 128) ? '-' : '+';
}

static int abs(uint8_t a)
{
	return (a & 128) ? (256 - a) : a;
}

static std::string toHex(unsigned value, unsigned width)
{
	std::ostringstream s;
	s << std::hex << std::setw(width) << std::setfill('0') << value;
	return s.str();
}

static std::string translateAddress(
	int address, MemoryLayout* memLayout, SymbolTable* symTable)
{
	if (Symbol* label = symTable->getAddressSymbol(address, memLayout)) {
		return label->text().toStdString();
	} else {
		return '#' + toHex(address, 4);
	}
}

static int get16(const uint8_t* memBuf, int address)
{
	return memBuf[address] + 256 * memBuf[address + 1];
}

void dasm(const uint8_t* membuf, uint16_t startAddr, uint16_t endAddr,
          DisasmLines& disasm, MemoryLayout* memLayout, SymbolTable* symTable, int currentPC)
{
	int pc = startAddr;
	int labelCount = 0;
	Symbol* symbol = symTable->findFirstAddressSymbol(pc, memLayout);

	disasm.clear();
	while (pc <= int(endAddr)) {
		// check for a label
		while (symbol && symbol->value() == pc) {
			++labelCount;
			DisasmRow destsym;
			destsym.rowType = DisasmRow::LABEL;
			destsym.numBytes = 0;
			destsym.infoLine = labelCount;
			destsym.addr = pc;
			destsym.instr = symbol->text().toStdString();
			disasm.push_back(destsym);
			symbol = symTable->findNextAddressSymbol(memLayout);
		}

		labelCount = 0;
		DisasmRow dest;
		dest.rowType = DisasmRow::INSTRUCTION;
		dest.addr = pc;
		dest.numBytes = 0;
		dest.infoLine = 0;
		dest.instr.clear();

		const char* s;
		const char* r = nullptr;
		switch (membuf[pc]) {
		case 0xCB:
			s = mnemonic_cb[membuf[pc + 1]];
			dest.numBytes = 2;
			break;
		case 0xED:
			s = mnemonic_ed[membuf[pc + 1]];
			dest.numBytes = 2;
			break;
		case 0xDD:
		case 0xFD:
			r = (membuf[pc] == 0xDD) ? "ix" : "iy";
			if (membuf[pc + 1] != 0xcb) {
				s = mnemonic_xx[membuf[pc + 1]];
				dest.numBytes = 2;
			} else {
				s = mnemonic_xx_cb[membuf[pc + 3]];
				dest.numBytes = 4;
			}
			break;
		default:
			s = mnemonic_main[membuf[pc]];
			dest.numBytes = 1;
		}

		for (int j = 0; s[j]; ++j) {
			switch (s[j]) {
			case 'A': {
				int address = get16(membuf, pc + dest.numBytes);
				dest.instr += translateAddress(address, memLayout, symTable);
				dest.numBytes += 2;
				break;
			}
			case 'B':
				dest.instr += '#' + toHex(membuf[pc + dest.numBytes], 2);
				dest.numBytes += 1;
				break;
			case 'R': {
				int address = (pc + 2 + (signed char)membuf[pc + dest.numBytes]) & 0xFFFF;
				dest.instr += translateAddress(address, memLayout, symTable);
				dest.numBytes += 1;
				break;
			}
			case 'W':
				dest.instr += '#' + toHex(get16(membuf, pc + dest.numBytes), 4);
				dest.numBytes += 2;
				break;
			case 'X': {
				uint8_t offset = membuf[pc + dest.numBytes];
				dest.instr += '(' + std::string(r) + sign(offset)
				           +  '#' + toHex(abs(offset), 2) + ')';
				dest.numBytes += 1;
				break;
			}
			case 'Y': {
				uint8_t offset = membuf[pc + 2];
				dest.instr += '(' + std::string(r) + sign(offset)
				           +  '#' + toHex(abs(offset), 2) + ')';
				break;
			}
			case 'I':
				dest.instr += r;
				break;
			case '!':
				dest.instr = "db     #ED,#" + toHex(membuf[pc + 1], 2);
				dest.numBytes = 2;
				break;
			case '@':
				dest.instr = "db     #" + toHex(membuf[pc], 2);
				dest.numBytes = 1;
				break;
			case '#':
				dest.instr = "db     #" + toHex(membuf[pc + 0], 2) +
				                "#CB,#" + toHex(membuf[pc + 2], 2);
				dest.numBytes = 2;
				break;
			case ' ': {
				dest.instr.resize(7, ' ');
				break;
			}
			default:
				dest.instr += s[j];
				break;
			}
		}

		// handle overflow at end or label
		int dataBytes = 0;
		if (symbol && pc + dest.numBytes > symbol->value()) {
			dataBytes = symbol->value() - pc;
		} else if (pc + dest.numBytes > endAddr) {
			dataBytes = endAddr - pc;
		} else if (pc + dest.numBytes > currentPC) {
			dataBytes = currentPC - pc;
		}

		switch (dataBytes) {
		case 1:
			dest.instr = "db     #" + toHex(membuf[pc + 0], 2);
			dest.numBytes = 1;
			break;
		case 2:
			dest.instr = "db     #" + toHex(membuf[pc + 0], 2) +
			                   ",#" + toHex(membuf[pc + 1], 2);
			dest.numBytes = 2;
			break;
		case 3:
			dest.instr = "db     #" + toHex(membuf[pc + 0], 2) +
			                   ",#" + toHex(membuf[pc + 1], 2) +
			                   ",#" + toHex(membuf[pc + 2], 2);
			dest.numBytes = 3;
			break;
		default:
			break;
		}

		if (dest.instr.size() < 8) dest.instr.resize(8, ' ');
		disasm.push_back(dest);
		pc += dest.numBytes;
	}
}
