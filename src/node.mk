# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	DockableWidget DockableWidgetArea DockableWidgetLayout \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager \
	Settings PreferencesDialog BreakpointDialog DebuggableViewer \
	DebugSession MainMemoryViewer

SRC_HDR:= \
	DockManager Dasm DasmTables DebuggerData SymbolTable Convert Version

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager PreferencesDialog BreakpointDialog

include build/node-end.mk
