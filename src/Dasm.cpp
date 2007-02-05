// $Id$

#include "Dasm.h"
#include "DasmTables.h"
#include "DebuggerData.h"
#include "SymbolTable.h"
#include <sstream>
#include <iomanip>

static char sign(unsigned char a)
{
	return (a & 128) ? '-' : '+';
}

static int abs(unsigned char a)
{
	return (a & 128) ? (256 - a) : a;
}

static std::string toHex(unsigned value, unsigned width)
{
	std::ostringstream s;
	s << std::hex << std::setw(width) << std::setfill('0') << value;
	return s.str();
}


void dasm(const unsigned char* membuf, unsigned short startAddr,
          unsigned short endAddr, DisasmLines& disasm, 
          MemoryLayout *memLayout, SymbolTable *symTable)
{
	const char* s;
	const char* r = 0;
	int pc = startAddr;
	int labelCount;
	DisasmRow dest;
	Symbol *symbol;
	
	disasm.clear();
	symbol = symTable->findFirstAddressSymbol( pc, memLayout );
	while (pc <= int(endAddr)) {
		// check for a label
		while( symbol && symbol->value() == pc ) {
			dest.rowType = DisasmRow::LABEL;
			labelCount++;
			dest.numBytes = 0;
			dest.infoLine = labelCount;
			dest.addr = pc;
			dest.instr = symbol->text().toAscii().data();
			disasm.push_back(dest);
			symbol = symTable->findNextAddressSymbol( memLayout );
		}
		
		labelCount = 0;
		dest.rowType = DisasmRow::INSTRUCTION;
		dest.addr = pc;
		dest.numBytes = 0;
		dest.infoLine = 0;
		dest.instr.clear();
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
				r = "ix";
				if (membuf[pc + 1] != 0xcb) {
					s = mnemonic_xx[membuf[pc + 1]];
					dest.numBytes = 2;
				} else {
					s = mnemonic_xx_cb[membuf[pc + 3]];
					dest.numBytes = 4;
				}
				break;
			case 0xFD:
				r = "iy";
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
			case 'B':
				dest.instr += '#' + toHex(membuf[pc + dest.numBytes], 2);
				dest.numBytes += 1;
				break;
			case 'R':
				dest.instr += '#' + toHex((pc + 2 + (signed char)membuf[pc + dest.numBytes]) & 0xFFFF, 4);
				dest.numBytes += 1;
				break;
			case 'W':
				dest.instr += '#' + toHex(membuf[pc + dest.numBytes] + membuf[pc + dest.numBytes + 1] * 256, 4);
				dest.numBytes += 2;
				break;
			case 'X': {
				unsigned char offset = membuf[pc + dest.numBytes];
				dest.instr += '(' + std::string(r) + sign(offset)
				           +  '#' + toHex(abs(offset), 2) + ')';
				dest.numBytes += 1;
				break;
			}
			case 'Y': {
				unsigned char offset = membuf[pc + 2];
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
			case '@':
				dest.instr = "db     #" + toHex(membuf[pc], 2);
				dest.numBytes = 1;
			case '#':
				dest.instr = "db     #" + toHex(membuf[pc + 0], 2) +
				                "#CB,#" + toHex(membuf[pc + 2], 2);
				dest.numBytes = 2;
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
		if(symbol && pc+dest.numBytes>symbol->value())
			dataBytes = symbol->value() - pc;
		else if( pc+dest.numBytes>endAddr )
			dataBytes = endAddr-pc;
			
		switch(dataBytes) {
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
				                   ",#" + toHex(membuf[pc + 1], 2);
				                   ",#" + toHex(membuf[pc + 2], 2);
				dest.numBytes = 3;
				break;
			default:
				break;
		}
		dest.instr.resize(19, ' ');
		disasm.push_back(dest);
		pc += dest.numBytes;
	}
}
