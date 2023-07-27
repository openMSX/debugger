cmake_minimum_required(VERSION 3.18)
project(openmsx-debugger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# find qt dependency
list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
find_package(Qt6 6.6.0 COMPONENTS Core Widgets Network REQUIRED)

# find python3 interpreter
find_package (Python3 COMPONENTS Interpreter)

set(HEADERS
    src/BitMapViewer.h
    src/BreakpointDialog.h
    src/BreakpointViewer.h
    src/CPURegs.h
    src/CPURegsViewer.h
    src/CommClient.h
    src/CommandDialog.h
    src/ConnectDialog.h
    src/Convert.h
    src/Dasm.h
    src/DasmTables.h
    src/DebugSession.h
    src/DebuggableViewer.h
    src/DebuggerData.h
    src/DebuggerForm.h
    src/DisasmViewer.h
    src/DockManager.h
    src/DockableWidget.h
    src/DockableWidgetArea.h
    src/DockableWidgetLayout.h
    src/FlagsViewer.h
    src/GotoDialog.h
    src/HexViewer.h
    src/InteractiveButton.h
    src/InteractiveLabel.h
    src/MainMemoryViewer.h
    src/OpenMSXConnection.h
    src/PaletteDialog.h
    src/PreferencesDialog.h
    src/ScopedAssign.h
    src/Settings.h
    src/SimpleHexRequest.h
    src/SlotViewer.h
    src/SpriteViewer.h
    src/StackViewer.h
    src/SymbolManager.h
    src/SymbolTable.h
    src/TileViewer.h
    src/VDPCommandRegViewer.h
    src/VDPDataStore.h
    src/VDPRegViewer.h
    src/VDPStatusRegViewer.h
    src/Version.h
    src/VramBitMappedView.h
    src/VramSpriteView.h
    src/VramTiledView.h
    src/ranges.h
    )
    
set(SOURCES
    resources/resources.qrc
    src/BitMapViewer.cpp
    src/BreakpointDialog.cpp
    src/BreakpointViewer.cpp
    src/CPURegs.cpp
    src/CPURegsViewer.cpp
    src/CommClient.cpp
    src/CommandDialog.cpp
    src/ConnectDialog.cpp
    src/Convert.cpp
    src/Dasm.cpp
    src/DasmTables.cpp
    src/DebugSession.cpp
    src/DebuggableViewer.cpp
    src/DebuggerData.cpp
    src/DebuggerForm.cpp
    src/DisasmViewer.cpp
    src/DockManager.cpp
    src/DockableWidget.cpp
    src/DockableWidgetArea.cpp
    src/DockableWidgetLayout.cpp
    src/FlagsViewer.cpp
    src/GotoDialog.cpp
    src/HexViewer.cpp
    src/InteractiveButton.cpp
    src/InteractiveLabel.cpp
    src/MainMemoryViewer.cpp
    src/OpenMSXConnection.cpp
    src/PaletteDialog.cpp
    src/PreferencesDialog.cpp
    src/Settings.cpp
    src/SimpleHexRequest.cpp
    src/SlotViewer.cpp
    src/SpriteViewer.cpp
    src/StackViewer.cpp
    src/SymbolManager.cpp
    src/SymbolTable.cpp
    src/TileViewer.cpp
    src/VDPCommandRegViewer.cpp
    src/VDPDataStore.cpp
    src/VDPRegViewer.cpp
    src/VDPStatusRegViewer.cpp
    src/Version.cpp
    src/VramBitMappedView.cpp
    src/VramSpriteView.cpp
    src/VramTiledView.cpp
    src/main.cpp
    )

# compile debugger
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(openmsx-debugger ${HEADERS} ${SOURCES} ${MOCS})
set_target_properties(openmsx-debugger PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_include_directories(openmsx-debugger PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(openmsx-debugger Qt6::Widgets Qt6::Network)
target_compile_options(openmsx-debugger PRIVATE -Werror -Wall -Wextra)

# but first create version include file
add_custom_target(
   version ALL
   COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/version2code.py ${CMAKE_BINARY_DIR}/include/Version.ii
   )
target_include_directories(openmsx-debugger PRIVATE ${CMAKE_BINARY_DIR}/include)
add_dependencies(openmsx-debugger version)
