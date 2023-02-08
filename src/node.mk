include build/node-start.mk

SUBDIRS:= \
	openmsx

MOC_SRC_HDR:= \
	DockableWidget DockableWidgetArea DockableWidgetLayout \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager \
	Settings PreferencesDialog BreakpointDialog CommandDialog DebuggableViewer \
	DebugSession MainMemoryViewer BitMapViewer VramBitMappedView \
	VDPDataStore VDPStatusRegViewer VDPRegViewer InteractiveLabel \
	InteractiveButton VDPCommandRegViewer GotoDialog SymbolTable \
	TileViewer VramTiledView PaletteDialog VramSpriteView SpriteViewer \
	BreakpointViewer

SRC_HDR:= \
	DockManager Dasm DasmTables DebuggerData SymbolTable Convert Version \
	CPURegs SimpleHexRequest

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager PreferencesDialog BreakpointDialog \
	CommandDialog BitMapViewer VDPStatusRegisters VDPRegViewer \
	VDPCommandRegisters GotoDialog TileViewer PaletteDialog SpriteViewer \
	BreakpointViewer

include build/node-end.mk
