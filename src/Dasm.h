// $Id$

#ifndef _DASM_H
#define _DASM_H

#include <string>
#include <vector>

class MemoryLayout;
class SymbolTable;

struct DisasmRow {
	enum RowType { INSTRUCTION, LABEL };
	
	RowType rowType;
	unsigned short addr;
	char numBytes;
	std::string instr;
};

static const DisasmRow DISABLED_ROW = {DisasmRow::INSTRUCTION, 0, 1, "-       "};

typedef std::vector<DisasmRow> DisasmLines;

void dasm(const unsigned char* membuf, unsigned short startAddr,
          unsigned short endAddr, DisasmLines& disasm, 
          MemoryLayout *memLayout, SymbolTable *symTable);

#endif // _DASM_H
