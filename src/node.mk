# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager

SRC_HDR:= \
	Dasm DasmTables DebuggerData SymbolTable

SRC_ONLY:= \
	main

HDR_ONLY:= \
	version

UI:= \
	ConnectDialog SymbolManager

include build/node-end.mk
