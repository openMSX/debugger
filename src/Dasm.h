// $Id$

#ifndef _DASM_H
#define _DASM_H

#include <string>
#include <vector>

typedef struct {
	unsigned short addr;
	char numBytes;
	std::string instr;
} DisasmRow;

static const DisasmRow DISABLED_ROW = {0, 1, "-       "};

typedef std::vector<DisasmRow> DisasmLines;

void dasm(const unsigned char *membuf, unsigned short startAddr, unsigned short endAddr, DisasmLines& disasm);

#endif // _DASM_H
