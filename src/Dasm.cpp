// $Id$

#include "Dasm.h"
#include "DasmTables.h"

static char sign(unsigned char a)
{
	return (a & 128) ? '-' : '+';
}

static int abs(unsigned char a)
{
	return (a & 128) ? (256 - a) : a;
}

void dasm(const unsigned char *membuf, unsigned short startAddr, unsigned short endAddr, DisasmLines& disasm)
{
	const char* s;
	char tmp[10];
	const char* r = 0;
	int pc = startAddr;
	DisasmRow dest;
	
	disasm.clear();
	while (pc <= int(endAddr)) {
		dest.addr = pc;
		dest.numBytes = 0;
		dest.instr.clear();
		switch (membuf[pc]) {
			case 0xCB:
				s = mnemonic_cb[membuf[pc+1]];
				dest.numBytes = 2;
				break;
			case 0xED:
				s = mnemonic_ed[membuf[pc+1]];
				dest.numBytes = 2;
				break;
			case 0xDD:
				r = "ix";
				if (membuf[pc+1] != 0xcb) {
					s = mnemonic_xx[membuf[1]];
					dest.numBytes = 2;
				} else {
					s = mnemonic_xx_cb[membuf[pc+3]];
					dest.numBytes = 4;
				}
				break;
			case 0xFD:
				r = "iy";
				if (membuf[pc+1] != 0xcb) {
					s = mnemonic_xx[membuf[pc+1]];
					dest.numBytes = 2;
				} else {
					s = mnemonic_xx_cb[membuf[pc+3]];
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
				sprintf(tmp, "#%02x", membuf[pc + dest.numBytes]);
				dest.numBytes += 1;
				dest.instr += tmp;
				break;
			case 'R':
				sprintf(tmp, "#%04x", (pc + 2 + (signed char)membuf[pc + dest.numBytes]) & 0xFFFF);
				dest.numBytes += 1;
				dest.instr += tmp;
				break;
			case 'W':
				sprintf(tmp, "#%04x", membuf[pc + dest.numBytes] + membuf[pc + dest.numBytes + 1] * 256);
				dest.numBytes += 2;
				dest.instr += tmp;
				break;
			case 'X':
				sprintf(tmp, "(%s%c#%02x)", r, sign(membuf[pc + dest.numBytes]),
                                                abs(membuf[pc + dest.numBytes]));
				dest.numBytes += 1;
				dest.instr += tmp;
				break;
			case 'Y':
				sprintf(tmp, "(%s%c#%02x)", r, sign(membuf[pc+2]), abs(membuf[pc+2]));
				dest.instr += tmp;
				break;
			case 'I':
				dest.instr += r;
				break;
			case '!':
				sprintf(tmp, "db     #ED,#%02x", membuf[pc+1]);
				dest.instr = tmp;
				dest.numBytes = 2;
			case '@':
				sprintf(tmp, "db     #%02x", membuf[pc]);
				dest.instr = tmp;
				dest.numBytes = 1;
			case '#':
				sprintf(tmp, "db     #%02x,#CB,#%02x", membuf[pc], membuf[pc+2]);
				dest.instr = tmp;
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
		// handle overflow
		if(pc+dest.numBytes>endAddr) {
			switch(endAddr-pc) {
				case 1:
  					sprintf(tmp, "db     #%02x", membuf[pc]);
					dest.instr = tmp;				
					dest.numBytes = 1;
					break;
				case 2:
  					sprintf(tmp, "db     #%02x,#%02x", membuf[pc], membuf[pc+1]);
					dest.instr = tmp;				
					dest.numBytes = 2;
					break;
				case 3:
  					sprintf(tmp, "db     #%02x,#%02x,#%02x", membuf[pc], membuf[pc+1], membuf[pc+2]);
					dest.instr = tmp;				
					dest.numBytes = 3;
					break;
			}
		}
		dest.instr.resize(18, ' ');
		disasm.push_back(dest);
		pc += dest.numBytes;
	}

}
