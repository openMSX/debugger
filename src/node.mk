# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager \
	Settings PreferencesDialog BreakpointDialog

SRC_HDR:= \
	Dasm DasmTables DebuggerData SymbolTable Convert Version

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager PreferencesDialog BreakpointDialog

include build/node-end.mk
