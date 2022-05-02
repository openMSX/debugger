include build/node-start.mk

SUBDIRS:= \
	openmsx

MOC_SRC_HDR:= \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer ConnectDialog OpenMSXConnection SymbolManager \
	Settings PreferencesDialog BreakpointDialog DebuggableViewer \
	DebugSession MainMemoryViewer BitMapViewer VramBitMappedView \
	VDPDataStore VDPStatusRegViewer VDPRegViewer InteractiveLabel \
	InteractiveButton VDPCommandRegViewer GotoDialog SymbolTable \
	TileViewer VramTiledView PaletteDialog VramSpriteView SpriteViewer \
	BreakpointViewer SignalDispatcher \
	blendsplitter/BlendSplitter blendsplitter/Expander \
	blendsplitter/Overlay blendsplitter/SplitterDecorator \
	blendsplitter/SwitchingBar blendsplitter/SwitchingWidget \
	blendsplitter/WidgetRegistry blendsplitter/ExpanderCorner \
	blendsplitter/RegistryItem \
	blendsplitter/SplitterHandle blendsplitter/SwitchingCombo \
	blendsplitter/WidgetDecorator QuickGuide TabRenamerHelper



SRC_HDR:= \
	Dasm DasmTables DebuggerData SymbolTable Convert Version \
	CPURegs SimpleHexRequest

SRC_ONLY:= \
	main

UI:= \
	ConnectDialog SymbolManager PreferencesDialog BreakpointDialog \
	BitMapViewer VDPStatusRegisters VDPRegistersExplained VDPCommandRegisters \
	GotoDialog TileViewer PaletteDialog SpriteViewer BreakpointViewer \
	QuickGuide

include build/node-end.mk
