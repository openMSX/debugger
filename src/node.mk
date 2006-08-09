# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager

SRC_HDR:= \
	Dasm DasmTables DebuggerData SymbolTable Version

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager

include build/node-end.mk
