// $Id: $

#ifndef CPUREGS_H
#define CPUREGS_H

class CpuRegs
{
public:
	static const int REG_AF  ;
	static const int REG_AF2 ;
	static const int REG_BC  ;
	static const int REG_BC2 ;
	static const int REG_DE  ;
	static const int REG_DE2 ;
	static const int REG_HL  ;
	static const int REG_HL2 ;
	static const int REG_IX  ;
	static const int REG_IY  ;
	static const int REG_PC  ;
	static const int REG_SP  ;
	static const int REG_I   ;
	static const int REG_R   ;
	static const int REG_IM  ;
	static const int REG_IFF ;

	static const char* regNames[];
};
#endif //CPUREGS_H
