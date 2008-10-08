# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	DockableWidget DockableWidgetArea DockableWidgetLayout \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager \
	Settings PreferencesDialog BreakpointDialog DebuggableViewer \
	DebugSession MainMemoryViewer BitMapViewer VramBitMappedView \
	VDPDataStore VDPStatusRegViewer VDPRegViewer InteractiveLabel InteractiveButton

SRC_HDR:= \
	DockManager Dasm DasmTables DebuggerData SymbolTable Convert Version \
	CPURegs 

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager PreferencesDialog BreakpointDialog \
	BitMapViewer VDPStatusRegisters VDPRegistersExplained

UI_PROMO_HDR:= 
#	InteractiveLabel

include build/node-end.mk
