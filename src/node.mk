# $Id$

include build/node-start.mk

MOC_SRC_HDR:= \
	CPURegsViewer CommClient DebuggerForm DisasmViewer FlagsViewer HexViewer \
	SlotViewer StackViewer

SRC_HDR:= \
	Dasm DasmTables DebuggerData ServerList

SRC_ONLY:= \
	main

HDR_ONLY:= \
	version

include build/node-end.mk

