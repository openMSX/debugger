#ifndef DASM_H
#define DASM_H

#include <string>
#include <vector>

class SymbolTable;
struct MemoryLayout;

struct DisasmRow {
	enum RowType { INSTRUCTION, LABEL };

	RowType rowType;
	unsigned short addr;
	char numBytes;
	int infoLine;
	std::string instr;
};

static const DisasmRow DISABLED_ROW = {DisasmRow::INSTRUCTION, 0, 1, 0, "-       "};
static const int FIRST_INFO_LINE = 1;
static const int LAST_INFO_LINE = -65536;

using DisasmLines = std::vector<DisasmRow>;

void dasm(const uint8_t* membuf, uint16_t startAddr, uint16_t endAddr, DisasmLines& disasm,
          MemoryLayout *memLayout, SymbolTable *symTable, int currentPC);

#endif // DASM_H
