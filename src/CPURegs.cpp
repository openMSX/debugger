#include "CPURegs.h"

const int CpuRegs::REG_AF  = 0;
const int CpuRegs::REG_AF2 = 1;
const int CpuRegs::REG_BC  = 2;
const int CpuRegs::REG_BC2 = 3;
const int CpuRegs::REG_DE  = 4;
const int CpuRegs::REG_DE2 = 5;
const int CpuRegs::REG_HL  = 6;
const int CpuRegs::REG_HL2 = 7;
const int CpuRegs::REG_IX  = 8;
const int CpuRegs::REG_IY  = 9;
const int CpuRegs::REG_PC  = 10;
const int CpuRegs::REG_SP  = 11;
const int CpuRegs::REG_I   = 12;
const int CpuRegs::REG_R   = 13;
const int CpuRegs::REG_IM  = 14;
const int CpuRegs::REG_IFF = 15;

const char* const CpuRegs::regNames[14] = {
	"AF", "AF'", "BC", "BC'",
	"DE", "DE'", "HL", "HL'",
	"IX", "IY", "PC", "SP",
	"I", "R"
};
