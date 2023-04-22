#include "DebuggerForm.h"
#include "BitMapViewer.h"
#include "TileViewer.h"
#include "SpriteViewer.h"
#include "DockableWidget.h"
#include "DisasmViewer.h"
#include "MainMemoryViewer.h"
#include "CPURegsViewer.h"
#include "FlagsViewer.h"
#include "StackViewer.h"
#include "SlotViewer.h"
#include "BreakpointViewer.h"
#include "CommClient.h"
#include "ConnectDialog.h"
#include "SymbolManager.h"
#include "PreferencesDialog.h"
#include "BreakpointDialog.h"
#include "CommandDialog.h"
#include "GotoDialog.h"
#include "DebuggableViewer.h"
#include "VDPRegViewer.h"
#include "VDPStatusRegViewer.h"
#include "VDPCommandRegViewer.h"
#include "Settings.h"
#include "Version.h"
#include <QAction>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QSplitter>
#include <QPixmap>
#include <QFileDialog>
#include <QCloseEvent>
#include <iostream>

bool forceBreak = false;

class QueryPauseHandler : public SimpleCommand
{
public:
	QueryPauseHandler(DebuggerForm& form_)
		: SimpleCommand("set pause")
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		// old openmsx versions returned 'on','false'
		// new versions return 'true','false'
		// so check for 'false'
		bool checked = message.trimmed() != "false";
		form.systemPauseAction->setChecked(checked);
		delete this;
	}
private:
	DebuggerForm& form;
};


class QueryBreakedHandler : public SimpleCommand
{
public:
	QueryBreakedHandler(DebuggerForm& form_)
		: SimpleCommand("debug breaked")
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		form.finalizeConnection(message.trimmed() == "1");
		delete this;
	}
private:
	DebuggerForm& form;
};


class CPURegRequest : public ReadDebugBlockCommand
{
public:
	CPURegRequest(DebuggerForm& form_)
		: ReadDebugBlockCommand("{CPU regs}", 0, 28, buf)
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		copyData(message);
		form.regsView->setData(buf);
		delete this;
	}

private:
	DebuggerForm& form;
	uint8_t buf[28];
};


class ListDebuggablesHandler : public SimpleCommand
{
public:
	ListDebuggablesHandler(DebuggerForm& form_)
		: SimpleCommand("debug list")
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		form.setDebuggables(message);
		delete this;
	}
private:
	DebuggerForm& form;
};


class DebuggableSizeHandler : public SimpleCommand
{
public:
	DebuggableSizeHandler(const QString& debuggable_, DebuggerForm& form_)
		: SimpleCommand(QString("debug size %1").arg(debuggable_))
		, debuggable(debuggable_)
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		form.setDebuggableSize(debuggable, message.toInt());
		delete this;
	}
private:
	QString debuggable;
	DebuggerForm& form;
};


int DebuggerForm::counter = 0;

DebuggerForm::DebuggerForm(QWidget* parent)
	: QMainWindow(parent)
	, comm(CommClient::instance())
{
	VDPRegView = nullptr;
	VDPStatusRegView = nullptr;
	VDPCommandRegView = nullptr;

	createActions();
	createMenus();
	createToolbars();
	createStatusbar();
	createForm();

	recentFiles = Settings::get().value("MainWindow/RecentFiles").toStringList();
	updateRecentFiles();

	connect(&session.symbolTable(), &SymbolTable::symbolFileChanged, this, &DebuggerForm::symbolFileChanged);
}

void DebuggerForm::createActions()
{
	fileNewSessionAction = new QAction(tr("&New Session"), this);
	fileNewSessionAction->setStatusTip(tr("Clear the current session."));

	fileOpenSessionAction = new QAction(tr("&Open Session ..."), this);
	fileOpenSessionAction->setShortcut(tr("Ctrl+O"));
	fileOpenSessionAction->setStatusTip(tr("Clear the current session."));

	fileSaveSessionAction = new QAction(tr("&Save Session"), this);
	fileSaveSessionAction->setShortcut(tr("Ctrl+S"));
	fileSaveSessionAction->setStatusTip(tr("Save the the current debug session"));

	fileSaveSessionAsAction = new QAction(tr("Save Session &As"), this);
	fileSaveSessionAsAction->setStatusTip(tr("Save the debug session in a selected file"));

	fileQuitAction = new QAction(tr("&Quit"), this);
	fileQuitAction->setShortcut(tr("Ctrl+Q"));
	fileQuitAction->setStatusTip(tr("Quit the openMSX debugger"));

	for (auto& rfa : recentFileActions) {
		rfa = new QAction(this);
		connect(rfa, &QAction::triggered, this, &DebuggerForm::fileRecentOpen);
	}

	systemConnectAction = new QAction(tr("&Connect"), this);
	systemConnectAction->setShortcut(tr("Ctrl+C"));
	systemConnectAction->setStatusTip(tr("Connect to openMSX"));
	systemConnectAction->setIcon(QIcon(":/icons/connect.png"));

	systemDisconnectAction = new QAction(tr("&Disconnect"), this);
	systemDisconnectAction->setShortcut(tr(""));
	systemDisconnectAction->setStatusTip(tr("Disconnect from openMSX"));
	systemDisconnectAction->setIcon(QIcon(":/icons/disconnect.png"));
	systemDisconnectAction->setEnabled(false);

	systemPauseAction = new QAction(tr("&Pause emulator"), this);
	systemPauseAction->setShortcut(Qt::Key_Pause);
	systemPauseAction->setStatusTip(tr("Pause the emulation"));
	systemPauseAction->setIcon(QIcon(":/icons/pause.png"));
	systemPauseAction->setCheckable(true);
	systemPauseAction->setEnabled(false);

	systemRebootAction = new QAction(tr("&Reboot emulator"), this);
	systemRebootAction->setStatusTip(tr("Reboot the emulation and start if needed"));
	systemRebootAction->setEnabled(false);

	systemSymbolManagerAction = new QAction(tr("&Symbol manager ..."), this);
	systemSymbolManagerAction->setStatusTip(tr("Start the symbol manager"));
	systemSymbolManagerAction->setIcon(QIcon(":/icons/symmanager.png"));

	systemPreferencesAction = new QAction(tr("Pre&ferences ..."), this);
	systemPreferencesAction->setStatusTip(tr("Set the global debugger preferences"));

	searchGotoAction = new QAction(tr("&Goto ..."), this);
	searchGotoAction->setStatusTip(tr("Jump to a specific address or label in the disassembly view"));
	searchGotoAction->setShortcut(tr("Ctrl+G"));

	viewRegistersAction = new QAction(tr("CPU &Registers"), this);
	viewRegistersAction->setStatusTip(tr("Toggle the cpu registers display"));
	viewRegistersAction->setCheckable(true);

	viewBreakpointsAction = new QAction(tr("Debug &List"), this);
	viewBreakpointsAction->setStatusTip(tr("Toggle the breakpoints/watchpoints display"));
	viewBreakpointsAction->setCheckable(true);

	viewFlagsAction = new QAction(tr("CPU &Flags"), this);
	viewFlagsAction->setStatusTip(tr("Toggle the cpu flags display"));
	viewFlagsAction->setCheckable(true);

	viewStackAction = new QAction(tr("Stack"), this);
	viewStackAction->setStatusTip(tr("Toggle the stack display"));
	viewStackAction->setCheckable(true);

	viewSlotsAction = new QAction(tr("Slots"), this);
	viewSlotsAction->setStatusTip(tr("Toggle the slots display"));
	viewSlotsAction->setCheckable(true);

	viewMemoryAction = new QAction(tr("Memory"), this);
	viewMemoryAction->setStatusTip(tr("Toggle the main memory display"));
	viewMemoryAction->setCheckable(true);

	viewDebuggableViewerAction = new QAction(tr("Add debuggable viewer"), this);
	viewDebuggableViewerAction->setStatusTip(tr("Add a hex viewer for debuggables"));

	viewVDPStatusRegsAction = new QAction(tr("Status Registers"), this);
	viewVDPStatusRegsAction->setStatusTip(tr("The VDP status registers interpreted"));
	viewVDPStatusRegsAction->setCheckable(true);
	viewVDPCommandRegsAction = new QAction(tr("Command Registers"), this);
	viewVDPCommandRegsAction->setStatusTip(tr("Interact with the VDP command registers"));
	viewVDPCommandRegsAction->setCheckable(true);
	viewVDPRegsAction = new QAction(tr("Registers"), this);
	viewVDPRegsAction->setStatusTip(tr("Interact with the VDP registers"));
	viewVDPRegsAction->setCheckable(true);
	viewBitMappedAction = new QAction(tr("Bitmapped VRAM"), this);
	viewBitMappedAction->setStatusTip(tr("Decode VRAM as screen 5/6/7/8 image"));
	//viewBitMappedAction->setCheckable(true);
	viewCharMappedAction = new QAction(tr("Tiles in VRAM"), this);
	viewCharMappedAction->setStatusTip(tr("Decode VRAM as MSX1 screen tiles"));
	viewSpritesAction = new QAction(tr("Sprites in VRAM"), this);
	viewSpritesAction->setStatusTip(tr("Decode sprites tiles"));

	executeBreakAction = new QAction(tr("Break"), this);
	executeBreakAction->setShortcut(tr("CRTL+B"));
	executeBreakAction->setStatusTip(tr("Halt the execution and enter debug mode"));
	executeBreakAction->setIcon(QIcon(":/icons/break.png"));
	executeBreakAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeBreakAction->setEnabled(true); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeBreakAction->setEnabled(false); });

	executeRunAction = new QAction(tr("Run"), this);
	executeRunAction->setShortcut(tr("F9"));
	executeRunAction->setStatusTip(tr("Leave debug mode and resume execution"));
	executeRunAction->setIcon(QIcon(":/icons/run.png"));
	executeRunAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeRunAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeRunAction->setEnabled(true); });

	executeStepAction = new QAction(tr("Step into"), this);
	executeStepAction->setShortcut(tr("F7"));
	executeStepAction->setStatusTip(tr("Execute a single instruction"));
	executeStepAction->setIcon(QIcon(":/icons/stepinto.png"));
	executeStepAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeStepAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeStepAction->setEnabled(true); });

	executeStepOverAction = new QAction(tr("Step over"), this);
	executeStepOverAction->setShortcut(tr("F8"));
	executeStepOverAction->setStatusTip(tr("Execute the next instruction including any called subroutines"));
	executeStepOverAction->setIcon(QIcon(":/icons/stepover.png"));
	executeStepOverAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeStepOverAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeStepOverAction->setEnabled(true); });

	executeStepOutAction = new QAction(tr("Step out"), this);
	executeStepOutAction->setShortcut(tr("F11"));
	executeStepOutAction->setStatusTip(tr("Resume execution until the current routine has finished"));
	executeStepOutAction->setIcon(QIcon(":/icons/stepout.png"));
	executeStepOutAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeStepOutAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeStepOutAction->setEnabled(true); });

	executeStepBackAction = new QAction(tr("Step back"), this);
	executeStepBackAction->setShortcut(tr("F12"));
	executeStepBackAction->setStatusTip(tr("Reverse the last instruction"));
	executeStepBackAction->setIcon(QIcon(":/icons/stepback.png"));
	executeStepBackAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeStepBackAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeStepBackAction->setEnabled(true); });

	executeRunToAction = new QAction(tr("Run to"), this);
	executeRunToAction->setShortcut(tr("F4"));
	executeRunToAction->setStatusTip(tr("Resume execution until the selected line is reached"));
	executeRunToAction->setIcon(QIcon(":/icons/runto.png"));
	executeRunToAction->setEnabled(false);
	connect(this, &DebuggerForm::runStateEntered,   [this]{ executeRunToAction->setEnabled(false); });
	connect(this, &DebuggerForm::breakStateEntered, [this]{ executeRunToAction->setEnabled(true); });

	breakpointToggleAction = new QAction(tr("Toggle"), this);
	breakpointToggleAction->setShortcut(tr("F5"));
	breakpointToggleAction->setStatusTip(tr("Toggle breakpoint on/off at cursor"));
	breakpointToggleAction->setIcon(QIcon(":/icons/break.png"));
	breakpointToggleAction->setEnabled(false);

	commandAction = new QAction(tr("Manage command buttons..."), this);
	commandAction->setStatusTip(tr("Add new Tcl-scripted command button"));
	commandAction->setEnabled(false);

	breakpointAddAction = new QAction(tr("Add ..."), this);
	breakpointAddAction->setShortcut(tr("CTRL+B"));
	breakpointAddAction->setStatusTip(tr("Add a breakpoint at a location"));
	breakpointAddAction->setEnabled(false);

	helpAboutAction = new QAction(tr("&About"), this);
	helpAboutAction->setStatusTip(tr("Show the application information"));

	connect(fileNewSessionAction, &QAction::triggered, this, &DebuggerForm::fileNewSession);
	connect(fileOpenSessionAction, &QAction::triggered, this, &DebuggerForm::fileOpenSession);
	connect(fileSaveSessionAction, &QAction::triggered, this, &DebuggerForm::fileSaveSession);
	connect(fileSaveSessionAsAction, &QAction::triggered, this, &DebuggerForm::fileSaveSessionAs);
	connect(fileQuitAction, &QAction::triggered, this, &DebuggerForm::close);
	connect(systemConnectAction, &QAction::triggered, this, &DebuggerForm::systemConnect);
	connect(systemDisconnectAction, &QAction::triggered, this, &DebuggerForm::systemDisconnect);
	connect(systemPauseAction, &QAction::triggered, this, &DebuggerForm::systemPause);
	connect(systemRebootAction, &QAction::triggered, this, &DebuggerForm::systemReboot);
	connect(systemSymbolManagerAction, &QAction::triggered, this, &DebuggerForm::systemSymbolManager);
	connect(systemPreferencesAction, &QAction::triggered, this, &DebuggerForm::systemPreferences);
	connect(searchGotoAction, &QAction::triggered, this, &DebuggerForm::searchGoto);
	connect(viewRegistersAction, &QAction::triggered, this, &DebuggerForm::toggleRegisterDisplay);
	connect(viewBreakpointsAction, &QAction::triggered, this, &DebuggerForm::toggleBreakpointsDisplay);
	connect(viewFlagsAction, &QAction::triggered, this, &DebuggerForm::toggleFlagsDisplay);
	connect(viewStackAction, &QAction::triggered, this, &DebuggerForm::toggleStackDisplay);
	connect(viewSlotsAction, &QAction::triggered, this, &DebuggerForm::toggleSlotsDisplay);
	connect(viewMemoryAction, &QAction::triggered, this, &DebuggerForm::toggleMemoryDisplay);
	connect(viewDebuggableViewerAction, &QAction::triggered, this, &DebuggerForm::addDebuggableViewer);
	connect(viewBitMappedAction, &QAction::triggered, this, &DebuggerForm::toggleBitMappedDisplay);
	connect(viewCharMappedAction, &QAction::triggered, this, &DebuggerForm::toggleCharMappedDisplay);
	connect(viewSpritesAction, &QAction::triggered, this, &DebuggerForm::toggleSpritesDisplay);
	connect(viewVDPRegsAction, &QAction::triggered, this, &DebuggerForm::toggleVDPRegsDisplay);
	connect(viewVDPCommandRegsAction, &QAction::triggered, this, &DebuggerForm::toggleVDPCommandRegsDisplay);
	connect(viewVDPStatusRegsAction, &QAction::triggered, this, &DebuggerForm::toggleVDPStatusRegsDisplay);
	connect(executeBreakAction, &QAction::triggered, this, &DebuggerForm::executeBreak);
	connect(executeRunAction, &QAction::triggered, this, &DebuggerForm::executeRun);
	connect(executeStepAction, &QAction::triggered, this, &DebuggerForm::executeStep);
	connect(executeStepOverAction, &QAction::triggered, this, &DebuggerForm::executeStepOver);
	connect(executeRunToAction, &QAction::triggered, this, &DebuggerForm::executeRunTo);
	connect(executeStepOutAction, &QAction::triggered, this, &DebuggerForm::executeStepOut);
	connect(executeStepBackAction, &QAction::triggered, this, &DebuggerForm::executeStepBack);
	connect(breakpointToggleAction, &QAction::triggered, this, &DebuggerForm::toggleBreakpoint);
	connect(breakpointAddAction, &QAction::triggered, this, &DebuggerForm::addBreakpoint);
	connect(commandAction, &QAction::triggered, this, &DebuggerForm::manageCommandButtons);
	connect(helpAboutAction, &QAction::triggered, this, &DebuggerForm::showAbout);
}

void DebuggerForm::createMenus()
{
	// create file menu
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(fileNewSessionAction);
	fileMenu->addAction(fileOpenSessionAction);
	fileMenu->addAction(fileSaveSessionAction);
	fileMenu->addAction(fileSaveSessionAsAction);

	recentFileSeparator = fileMenu->addSeparator();
	for (auto* rfa : recentFileActions)
		fileMenu->addAction(rfa);

	fileMenu->addSeparator();
	fileMenu->addAction(fileQuitAction);

	// create system menu
	systemMenu = menuBar()->addMenu(tr("&System"));
	systemMenu->addAction(systemConnectAction);
	systemMenu->addAction(systemDisconnectAction);
	systemMenu->addSeparator();
	systemMenu->addAction(systemPauseAction);
	systemMenu->addSeparator();
	systemMenu->addAction(systemRebootAction);
	systemMenu->addSeparator();
	systemMenu->addAction(systemSymbolManagerAction);
	systemMenu->addSeparator();
	systemMenu->addAction(systemPreferencesAction);

	// create system menu
	searchMenu = menuBar()->addMenu(tr("Se&arch"));
	searchMenu->addAction(searchGotoAction);

	// create view menu
	viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(viewRegistersAction);
	viewMenu->addAction(viewFlagsAction);
	viewMenu->addAction(viewStackAction);
	viewMenu->addAction(viewSlotsAction);
	viewMenu->addAction(viewMemoryAction);
	viewMenu->addAction(viewBreakpointsAction);
	viewVDPDialogsMenu = viewMenu->addMenu("VDP");
	viewMenu->addSeparator();
	viewFloatingWidgetsMenu = viewMenu->addMenu("Floating widgets:");
	viewMenu->addAction(viewDebuggableViewerAction);
	connect(viewMenu, &QMenu::aboutToShow, this, &DebuggerForm::updateViewMenu);

	// create VDP dialogs menu
	viewVDPDialogsMenu->addAction(viewVDPRegsAction);
	viewVDPDialogsMenu->addAction(viewVDPCommandRegsAction);
	viewVDPDialogsMenu->addAction(viewVDPStatusRegsAction);
	viewVDPDialogsMenu->addAction(viewBitMappedAction);
	viewVDPDialogsMenu->addAction(viewCharMappedAction);
	viewVDPDialogsMenu->addAction(viewSpritesAction);
	connect(viewVDPDialogsMenu, &QMenu::aboutToShow, this, &DebuggerForm::updateVDPViewMenu);

	// create Debuggable Viewers menu (so the user can focus an existing one)
	connect(viewFloatingWidgetsMenu, &QMenu::aboutToShow, this, &DebuggerForm::updateViewFloatingWidgetsMenu);

	// create execute menu
	executeMenu = menuBar()->addMenu(tr("&Execute"));
	executeMenu->addAction(executeBreakAction);
	executeMenu->addAction(executeRunAction);
	executeMenu->addSeparator();
	executeMenu->addAction(executeStepAction);
	executeMenu->addAction(executeStepOverAction);
	executeMenu->addAction(executeStepOutAction);
	executeMenu->addAction(executeStepBackAction);
	executeMenu->addAction(executeRunToAction);

	// create breakpoint menu
	breakpointMenu = menuBar()->addMenu(tr("&Breakpoint"));
	breakpointMenu->addAction(breakpointToggleAction);
	breakpointMenu->addAction(breakpointAddAction);

	// create command menu
	commandMenu = menuBar()->addMenu("&Commands");
	commandMenu->addAction(commandAction);

	// create help menu
	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(helpAboutAction);
}

void DebuggerForm::createToolbars()
{
	// create debug toolbar
	systemToolbar = addToolBar(tr("System"));
	systemToolbar->addAction(systemConnectAction);
	systemToolbar->addAction(systemDisconnectAction);
	systemToolbar->addSeparator();
	systemToolbar->addAction(systemPauseAction);
	systemToolbar->addSeparator();
	systemToolbar->addAction(systemSymbolManagerAction);

	// create debug toolbar
	executeToolbar = addToolBar(tr("Execution"));
	executeToolbar->addAction(executeBreakAction);
	executeToolbar->addAction(executeRunAction);
	executeToolbar->addSeparator();
	executeToolbar->addAction(executeStepAction);
	executeToolbar->addAction(executeStepOverAction);
	executeToolbar->addAction(executeStepOutAction);
	executeToolbar->addAction(executeStepBackAction);
	executeToolbar->addAction(executeRunToAction);

	// create customised toolbar
	userToolbar = addToolBar(tr("User defined"));
	userToolbar->setEnabled(false);
	connect(&comm, &CommClient::connectionReady, userToolbar, [this]{ userToolbar->setEnabled(true); });
	connect(&comm, &CommClient::connectionTerminated, userToolbar, [this]{ userToolbar->setEnabled(false); });
	updateCustomActions();
}

void DebuggerForm::updateCustomActions()
{
	userToolbar->clear();
	for (const auto& command : commands) {
		auto* action = new QAction(command.name, this);
		action->setStatusTip(command.description);
		action->setIcon(QIcon(command.icon.isEmpty() ? ":/icons/gear.png" : command.icon));
		connect(action, &QAction::triggered, [this, command]{ comm.sendCommand(new SimpleCommand(command.source)); });
		userToolbar->addAction(action);
	}
}

void DebuggerForm::createStatusbar()
{
	// create the statusbar
	statusBar()->showMessage("No emulation running.");
}

void DebuggerForm::createForm()
{
	updateWindowTitle();

	mainArea = std::make_unique<DockableWidgetArea>();
	dockMan.addDockArea(mainArea.get());
	setCentralWidget(mainArea.get());

	// Create main widgets and append them to the list first
	auto* dw = new DockableWidget(dockMan);

	// create the disasm viewer widget
	disasmView = new DisasmViewer();
	dw->setWidget(disasmView);
	dw->setTitle(tr("Code view"));
	dw->setId("CODEVIEW");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(false);
	dw->setClosable(false);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create the memory view widget
	mainMemoryView = new MainMemoryViewer();
	dw = new DockableWidget(dockMan);
	dw->setWidget(mainMemoryView);
	dw->setTitle(tr("Main memory"));
	dw->setId("MEMORY");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create register viewer
	regsView = new CPURegsViewer();
	dw = new DockableWidget(dockMan);
	dw->setWidget(regsView);
	dw->setTitle(tr("CPU registers"));
	dw->setId("REGISTERS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create flags viewer
	flagsView = new FlagsViewer();
	dw = new DockableWidget(dockMan);
	dw->setWidget(flagsView);
	dw->setTitle(tr("Flags"));
	dw->setId("FLAGS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create stack viewer
	stackView = new StackViewer();
	dw = new DockableWidget(dockMan);
	dw->setWidget(stackView);
	dw->setTitle(tr("Stack"));
	dw->setId("STACK");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create slot viewer
	slotView = new SlotViewer();
	dw = new DockableWidget(dockMan);
	dw->setWidget(slotView);
	dw->setTitle(tr("Memory layout"));
	dw->setId("SLOTS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// create breakpoints viewer
	bpView = new BreakpointViewer(session, this);
	dw = new DockableWidget(dockMan);
	dw->setWidget(bpView);
	dw->setTitle(tr("Debug list"));
	dw->setId("DEBUG");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged, this, &DebuggerForm::dockWidgetVisibilityChanged);

	// restore layout
	restoreGeometry(Settings::get().value("Layout/WindowGeometry", saveGeometry()).toByteArray());

	QStringList list = Settings::get().value("Layout/WidgetLayout").toStringList();
	// defaults needed?
	if (list.empty() || !list.at(0).startsWith("CODEVIEW ")) {
		list.clear();
		list.append("CODEVIEW D V R 0 -1 -1");
		list.append("REGISTERS D V R 0 -1 -1");
		list.append("FLAGS D V R 0 -1 -1");
		int regW  = dockMan.findDockableWidget("REGISTERS")->sizeHint().width();
		int regH  = dockMan.findDockableWidget("REGISTERS")->sizeHint().height();
		list.append(QString("SLOTS D V R 0 -1 %1").arg(regH));
		int codeW = dockMan.findDockableWidget("CODEVIEW")->sizeHint().width();
		int codeH = dockMan.findDockableWidget("CODEVIEW")->sizeHint().height();
		int flagW = dockMan.findDockableWidget("FLAGS")->sizeHint().width();
		int slotW = dockMan.findDockableWidget("SLOTS")->sizeHint().width();
		list.append(QString("STACK D V R 0 -1 %1").arg(codeH));
		list.append(QString("MEMORY D V B %1 %2 %3").arg(codeW)
		                                             .arg(regW + flagW + slotW)
		                                             .arg(codeH - regH));
		int stackW = dockMan.findDockableWidget("STACK")->sizeHint().width();
		list.append(QString("DEBUG D V B %1 %2 -1").arg(codeW)
		                                             .arg(regW + flagW + slotW + stackW));
	}

	// restore commands
	restoreCommands(Settings::get().value("Commands/Tcl", saveCommands()).toByteArray());
	updateCustomActions();

	// add widgets
	for (int i = 0; i < list.size(); ++i) {
		QStringList s = list.at(i).split(" ", Qt::SplitBehaviorFlags::SkipEmptyParts);
		// get widget
		if ((dw = dockMan.findDockableWidget(s.at(0)))) {
			if (s.at(1) == "D") {
				// dock widget
				DockableWidgetLayout::DockSide side;
				if        (s.at(3) == "T") {
					side = DockableWidgetLayout::TOP;
				} else if (s.at(3) == "L") {
					side = DockableWidgetLayout::LEFT;
				} else if (s.at(3) == "R") {
					side = DockableWidgetLayout::RIGHT;
				} else {
					side = DockableWidgetLayout::BOTTOM;
				}
				dockMan.insertWidget(dw, 0, side, s.at(4).toInt(),
				                     s.at(5).toInt(), s.at(6).toInt());
				if (s.at(2) == "H") dw->hide();
			} else if (s.at(1) == "F") {
				//  float widget
				dw->setFloating(true, s.at(2) == "V");
				dw->resize(s.at(5).toInt(), s.at(6).toInt());
				dw->move  (s.at(3).toInt(), s.at(4).toInt());
			}
		}
	}

	// disable all widgets
	connectionClosed();

	// Disasm viewer
	connect(disasmView, &DisasmViewer::breakpointToggled, this, &DebuggerForm::toggleBreakpointAddress);
	connect(this, &DebuggerForm::symbolsChanged, disasmView, &DisasmViewer::refresh);
	connect(this, &DebuggerForm::settingsChanged, disasmView, &DisasmViewer::updateLayout);

	// Main memory viewer
	connect(this, &DebuggerForm::connected, mainMemoryView, &MainMemoryViewer::refresh);
	connect(this, &DebuggerForm::breakStateEntered, mainMemoryView, &MainMemoryViewer::refresh);

	// Slot viewer
	connect(this, &DebuggerForm::connected, slotView, &SlotViewer::refresh);
	connect(this, &DebuggerForm::breakStateEntered, slotView, &SlotViewer::refresh);
	// Received status update back from widget after breakStateEntered/connected
	connect(slotView, &SlotViewer::slotsUpdated, this, &DebuggerForm::onSlotsUpdated);

	// Breakpoint viewer
	connect(this, &DebuggerForm::breakpointsUpdated, bpView, &BreakpointViewer::refresh);
	connect(this, &DebuggerForm::runStateEntered, bpView, &BreakpointViewer::setRunState);
	connect(this, &DebuggerForm::breakStateEntered, bpView, &BreakpointViewer::setBreakState);

	// CPU regs viewer
	// Hook up the register viewer with the main memory viewer
	connect(regsView, &CPURegsViewer::registerChanged, mainMemoryView, &MainMemoryViewer::registerChanged);
	connect(regsView, &CPURegsViewer::flagsChanged, flagsView, &FlagsViewer::setFlags);
	connect(regsView, &CPURegsViewer::spChanged, stackView, &StackViewer::setStackPointer);
	// Received status update back from widgets after update
	connect(regsView, &CPURegsViewer::pcChanged, this, &DebuggerForm::onPCChanged);

	connect(&comm, &CommClient::connectionReady, this, &DebuggerForm::initConnection);
	connect(&comm, &CommClient::updateParsed, this, &DebuggerForm::handleUpdate);
	connect(&comm, &CommClient::connectionTerminated, this, &DebuggerForm::connectionClosed);

	// init main memory
	session.breakpoints().setMemoryLayout(&memLayout);
	disasmView->setMemory(mainMemory);
	disasmView->setBreakpoints(&session.breakpoints());
	disasmView->setMemoryLayout(&memLayout);
	disasmView->setSymbolTable(&session.symbolTable());
	mainMemoryView->setRegsView(regsView);
	mainMemoryView->setSymbolTable(&session.symbolTable());
	mainMemoryView->setDebuggable("memory", 0x10000);
	stackView->setData(mainMemory, 0x10000);
	slotView->setMemoryLayout(&memLayout);
	bpView->setBreakpoints(&session.breakpoints());
}

void DebuggerForm::closeEvent(QCloseEvent* e)
{
	// handle unsaved session
	fileNewSession();
	// cancel if session is still modified
	if (session.isModified()) {
		e->ignore();
		return;
	}

	// store commands
	Settings::get().setValue("Commands/Tcl", saveCommands());

	// store layout
	Settings::get().setValue("Layout/WindowGeometry", saveGeometry());

	QStringList layoutList;
	// fill layout list with docked widgets
	dockMan.getConfig(0, layoutList);
	// append floating widgets
	for (auto* widget : dockMan.managedWidgets()) {
		if (widget->isFloating()) {
			QString s("%1 F %2 %3 %4 %5 %6");
			s = s.arg(widget->id());
			if (widget->isHidden()) {
				s = s.arg("H");
			} else {
				s = s.arg("V");
			}
			s = s.arg(widget->x()).arg(widget->y())
			     .arg(widget->width()).arg(widget->height());
			layoutList.append(s);
		}
		widget->hide();
	}
	Settings::get().setValue("Layout/WidgetLayout", layoutList);

	QMainWindow::closeEvent(e);
}

void DebuggerForm::updateRecentFiles()
{
	// store settings
	Settings::get().setValue("MainWindow/RecentFiles", recentFiles);
	// update actions
	for (int i = 0; i < MaxRecentFiles; i++)
		if (i < recentFiles.size()) {
			recentFileActions[i]->setVisible(true);
			QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
			recentFileActions[i]->setText(text);
			recentFileActions[i]->setData(recentFiles[i]);
		} else {
			recentFileActions[i]->setVisible(false);
		}
	// show separator only when recent files exist
	recentFileSeparator->setVisible(!recentFiles.empty());
}

void DebuggerForm::addRecentFile(const QString& file)
{
	recentFiles.removeAll(file);
	recentFiles.prepend(file);
	while (recentFiles.size() > MaxRecentFiles)
		recentFiles.removeLast();
	updateRecentFiles();
}

void DebuggerForm::removeRecentFile(const QString& file)
{
	recentFiles.removeAll(file);
	updateRecentFiles();
}

void DebuggerForm::updateWindowTitle()
{
	QString title = "openMSX debugger [%1%2]";
	if (session.existsAsFile()) {
		title = title.arg(session.filename());
	} else {
		title = title.arg("unnamed session");
	}
	if (session.isModified()) {
		title = title.arg('*');
	} else {
		title = title.arg("");
	}
	setWindowTitle(title);
}

void DebuggerForm::initConnection()
{
	systemConnectAction->setEnabled(false);
	systemDisconnectAction->setEnabled(true);

	comm.sendCommand(new QueryPauseHandler(*this));
	comm.sendCommand(new QueryBreakedHandler(*this));

	comm.sendCommand(new SimpleCommand("openmsx_update enable status"));

	auto* command = new Command("openmsx_update enable debug",
		[=](const QString& /*message*/) {},
		[=](const QString& /*error*/) {
			// force reload if debug is disabled
			connect(bpView, &BreakpointViewer::contentsUpdated, this, [this]{ reloadBreakpoints(false); });
		});
	comm.sendCommand(command);

	comm.sendCommand(new ListDebuggablesHandler(*this));

	// define 'debug_bin2hex' proc for internal use
	comm.sendCommand(new SimpleCommand(
		"proc debug_bin2hex { input } {\n"
		"  set result \"\"\n"
		"  foreach i [split $input {}] {\n"
		"    append result [format %02X [scan $i %c]] \"\"\n"
		"  }\n"
		"  return $result\n"
		"}\n"));

	// define 'debug_hex2bin' proc for internal use
	comm.sendCommand(new SimpleCommand(
		"proc debug_hex2bin { input } {\n"
		"  set result \"\"\n"
		"  foreach {h l} [split $input {}] {\n"
		"    append result [binary format H2 $h$l] \"\"\n"
		"  }\n"
		"  return $result\n"
		"}\n"));

	// define 'debug_memmapper' proc for internal use
	comm.sendCommand(new SimpleCommand(
		"proc debug_memmapper { } {\n"
		"  set result \"\"\n"
		"  for { set page 0 } { $page &lt; 4 } { incr page } {\n"
		"    set tmp [get_selected_slot $page]\n"
		"    append result [lindex $tmp 0] [lindex $tmp 1] \"\\n\"\n"
		"    if { [lsearch [debug list] \"MapperIO\"] != -1} {\n"
		"      append result [debug read \"MapperIO\" $page] \"\\n\"\n"
		"    } else {\n"
		"      append result \"0\\n\"\n"
		"    }\n"
		"  }\n"
		"  for { set ps 0 } { $ps &lt; 4 } { incr ps } {\n"
		"    if [machine_info issubslotted $ps] {\n"
		"      append result \"1\\n\"\n"
		"      for { set ss 0 } { $ss &lt; 4 } { incr ss } {\n"
		"        append result [get_mapper_size $ps $ss] \"\\n\"\n"
		"      }\n"
		"    } else {\n"
		"      append result \"0\\n\"\n"
		"      append result [get_mapper_size $ps 0] \"\\n\"\n"
		"    }\n"
		"  }\n"
		"  for { set page 0 } { $page &lt; 4 } { incr page } {\n"
		"    set tmp [get_selected_slot $page]\n"
		"    set ss [lindex $tmp 1]\n"
		"    if { $ss == \"X\" } { set ss 0 }\n"
		"    set device_list [machine_info slot [lindex $tmp 0] $ss $page]\n"
		"    set name \"[lindex $device_list 0] romblocks\"\n"
		"    if { [lsearch [debug list] $name] != -1} {\n"
		"      append result \"[debug read $name [expr {$page * 0x4000}] ]\\n\"\n"
		"      append result \"[debug read $name [expr {$page * 0x4000 + 0x2000}] ]\\n\"\n"
		"    } else {\n"
		"      append result \"X\\nX\\n\"\n"
		"    }\n"
		"  }\n"
		"  return $result\n"
		"}\n"));

	// define 'debug_list_all_breaks' proc for internal use
	comm.sendCommand(new SimpleCommand(
		"proc debug_list_all_breaks { } {\n"
		"  set result [debug list_bp]\n"
		"  append result [debug list_watchpoints]\n"
		"  append result [debug list_conditions]\n"
		"  return $result\n"
		"}\n"));

	// define 'debug_check_debuggables' proc for internal use
	comm.sendCommand(new SimpleCommand(
		"proc debug_check_debuggables { debuggables } {\n"
		"  set all_debuggables [debug list]\n"
		"  lmap x $debuggables {expr {[lsearch $all_debuggables $x] >= 0}}\n"
		"}\n"));

}

void DebuggerForm::connectionClosed()
{
	systemPauseAction->setEnabled(false);
	systemRebootAction->setEnabled(false);
	executeBreakAction->setEnabled(false);
	executeRunAction->setEnabled(false);
	executeStepAction->setEnabled(false);
	executeStepOverAction->setEnabled(false);
	executeStepOutAction->setEnabled(false);
	executeStepBackAction->setEnabled(false);
	executeRunToAction->setEnabled(false);
	systemDisconnectAction->setEnabled(false);
	systemConnectAction->setEnabled(true);
	breakpointToggleAction->setEnabled(false);
	breakpointAddAction->setEnabled(false);
	commandAction->setEnabled(false);

	for (auto* w : dockMan.managedWidgets()) {
		w->widget()->setEnabled(false);
	}
}

void DebuggerForm::finalizeConnection(bool halted)
{
	systemPauseAction->setEnabled(true);
	systemRebootAction->setEnabled(true);
	breakpointToggleAction->setEnabled(true);
	breakpointAddAction->setEnabled(true);
	commandAction->setEnabled(true);

	// merge breakpoints on connect
	mergeBreakpoints = true;
	if (halted) {
		breakOccured();
	} else {
		setRunMode();
		updateData();
	}

	emit connected();

	for (auto* w : dockMan.managedWidgets()) {
		w->widget()->setEnabled(true);
	}
}

void DebuggerForm::handleUpdate(const QString& type, const QString& name,
                                const QString& message)
{
	if (type == "debug") {
		// TODO: read only new breakpoint
		reloadBreakpoints(false);
	} else if (type == "status") {
		if (name == "cpu") {
			// running state by default.
			if (message == "suspended") {
				breakOccured();
			}
		} else if (name == "paused") {
			pauseStatusChanged(message == "true");
		}
	}
}

void DebuggerForm::pauseStatusChanged(bool isPaused)
{
	systemPauseAction->setChecked(isPaused);
}

void DebuggerForm::breakOccured()
{
	emit breakStateEntered();
	updateData();
}

void DebuggerForm::updateData()
{
	reloadBreakpoints(mergeBreakpoints);
	// only merge the first time after connect
	mergeBreakpoints = false;

	// update registers
	// note that a register update is processed, a signal is sent to other
	// widgets as well. Any dependent updates shoud be called before this one.
	auto* regs = new CPURegRequest(*this);
	comm.sendCommand(regs);
}

void DebuggerForm::setRunMode()
{
	emit runStateEntered();
}

void DebuggerForm::fileNewSession()
{
	if (session.isModified()) {
		// save current session?
		int choice = QMessageBox::warning(this, tr("Unsaved session"),
		                tr("The current session has unsaved data.\n"
		                   "Do you want to save your changes?"),
		                QMessageBox::Save | QMessageBox::Discard |
		                QMessageBox::Cancel, QMessageBox::Save);
		if (choice == QMessageBox::Cancel) {
			return;
		} else if (choice == QMessageBox::Save) {
			fileSaveSession();
			// skip new if session is still modified (save was cancelled)
			if (session.isModified()) return;
		}
	}
	session.clear();
	updateWindowTitle();
}

void DebuggerForm::fileOpenSession()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open debug session"),
		QDir::currentPath(), tr("Debug Session Files (*.omds)"));

	if (!fileName.isEmpty())
		openSession(fileName);
}

void DebuggerForm::openSession(const QString& file)
{
	fileNewSession();
	session.open(file);
	if (systemDisconnectAction->isEnabled()) {
		// active connection, merge loaded breakpoints
		reloadBreakpoints(true);
	}
	// update recent
	if (session.existsAsFile()) {
		addRecentFile(file);
	} else {
		removeRecentFile(file);
	}

	updateWindowTitle();
}

void DebuggerForm::fileSaveSession()
{
	if (session.existsAsFile()) {
		session.save();
	} else {
		fileSaveSessionAs();
	}
	updateWindowTitle();
}

void DebuggerForm::fileSaveSessionAs()
{
	QFileDialog d(this, tr("Save debug session"));
	d.setNameFilter(tr("Debug Session Files (*.omds)"));
	d.setDefaultSuffix("omds");
	d.setDirectory(QDir::currentPath());
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	if (d.exec()) {
		session.saveAs(d.selectedFiles().at(0));
		// update recent
		if (session.existsAsFile()) {
			addRecentFile(session.filename());
		}
	}
	updateWindowTitle();
}

void DebuggerForm::fileRecentOpen()
{
	if (auto* action = qobject_cast<QAction *>(sender())) {
		openSession(action->data().toString());
	}
}

void DebuggerForm::systemConnect()
{
	if (auto connection = ConnectDialog::getConnection(this)) {
		comm.connectToOpenMSX(std::move(connection));
	}
}

void DebuggerForm::systemDisconnect()
{
	comm.closeConnection();
}

void DebuggerForm::systemPause()
{
	comm.sendCommand(new SimpleCommand(QString("set pause ") +
	                    (systemPauseAction->isChecked() ? "true" : "false")));
}

void DebuggerForm::systemReboot()
{
	if (systemPauseAction->isChecked()) {
		systemPauseAction->trigger();
	}
	if (executeRunAction->isEnabled()) {
		executeRun();
	}
	comm.sendCommand(new SimpleCommand("reset"));
}

void DebuggerForm::systemSymbolManager()
{
	symManager = new SymbolManager(session.symbolTable(), this);

	connect(this, &DebuggerForm::symbolFilesChanged,
	        disasmView, &DisasmViewer::refresh);
	connect(symManager, &SymbolManager::symbolTableChanged,
	        &session, &DebugSession::sessionModified);
	connect(symManager, &SymbolManager::symbolTableChanged,
	        bpView, &BreakpointViewer::onSymbolTableChanged);
	connect(this, &DebuggerForm::symbolFilesChanged,
	        symManager, &SymbolManager::refresh);
	connect(this, &DebuggerForm::symbolFilesChanged,
	        bpView, &BreakpointViewer::refresh);
	symManager->exec();

	emit symbolsChanged();
	updateWindowTitle();
}

void DebuggerForm::systemPreferences()
{
	PreferencesDialog prefs(this);
	prefs.exec();
	emit settingsChanged();
}

void DebuggerForm::searchGoto()
{
	GotoDialog gtd(memLayout, &session, this);
	if (gtd.exec()) {
		if (auto addr = gtd.address()) {
			disasmView->setCursorAddress(*addr, 0, DisasmViewer::MiddleAlways);
		}
	}
}

void DebuggerForm::executeBreak()
{
	comm.sendCommand(new SimpleCommand("debug break"));
}

void DebuggerForm::executeRun()
{
	comm.sendCommand(new SimpleCommand("debug cont"));
	setRunMode();
}

void DebuggerForm::executeStep()
{
	comm.sendCommand(new SimpleCommand("debug step"));
	setRunMode();
}

void DebuggerForm::executeStepOver()
{
	auto* sc = new Command("step_over",
		[this](const QString&){ finalizeConnection(true); });
	comm.sendCommand(sc);
	setRunMode();
}

void DebuggerForm::executeRunTo()
{
	comm.sendCommand(new SimpleCommand(
	                  "run_to " + QString::number(disasmView->cursorAddress())));
	setRunMode();
}

void DebuggerForm::executeStepOut()
{
	comm.sendCommand(new SimpleCommand("step_out"));
	setRunMode();
}

void DebuggerForm::executeStepBack()
{
	auto* sc = new Command("step_back",
		[this](const QString&){ finalizeConnection(true); });
	comm.sendCommand(sc);
	setRunMode();
}

void DebuggerForm::toggleBreakpoint()
{
	// address unspecified, use cursor address
	toggleBreakpointAddress(disasmView->cursorAddress());
}

void DebuggerForm::toggleBreakpointAddress(uint16_t addr)
{
	QString cmd;
	QString id;
	if (session.breakpoints().isBreakpoint(addr, &id)) {
		cmd = Breakpoints::createRemoveCommand(id);
	} else {
		// get slot
		auto [ps, ss, seg] = addressSlot(addr);
		// create command
		cmd = Breakpoints::createSetCommand(Breakpoint::BREAKPOINT, AddressRange{addr},
		                                    Slot{ps, ss}, seg);
	}
	comm.sendCommand(new SimpleCommand(cmd));
	// Get results from command above
	reloadBreakpoints();
}

void DebuggerForm::addBreakpoint()
{
	BreakpointDialog bpd(memLayout, &session, this);
	uint16_t addr = disasmView->cursorAddress();
	auto [ps, ss, seg] = addressSlot(addr);

	bpd.setData(Breakpoint::BREAKPOINT, AddressRange{addr}, Slot{ps, ss}, seg);

	if (bpd.exec()) {
		if (bpd.addressRange()) {
			QString cmd = Breakpoints::createSetCommand(
				bpd.type(), bpd.addressRange(), bpd.slot(), bpd.segment(), bpd.condition());
			comm.sendCommand(new SimpleCommand(cmd));
			// Get results of command above
			reloadBreakpoints();
		}
	}
}

void DebuggerForm::manageCommandButtons()
{
	commandDialog = new CommandDialog(commands, this);
	connect(commandDialog, &QDialog::finished, this, &DebuggerForm::manageCommandButtonsFinished);
	commandDialog->open();
}

void DebuggerForm::manageCommandButtonsFinished(int result)
{
	if (result == QDialog::Accepted)
		updateCustomActions();
}

void DebuggerForm::showAbout()
{
	QMessageBox::about(
		this, "openMSX Debugger", QString(Version::full().c_str()));
}

void DebuggerForm::toggleBreakpointsDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(bpView->parentWidget()));
}

void DebuggerForm::toggleRegisterDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(regsView->parentWidget()));
}

void DebuggerForm::toggleFlagsDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(flagsView->parentWidget()));
}

void DebuggerForm::toggleStackDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(stackView->parentWidget()));
}

void DebuggerForm::toggleSlotsDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(slotView->parentWidget()));
}

void DebuggerForm::toggleBitMappedDisplay()
{
	//toggleView(qobject_cast<DockableWidget*>(slotView->parentWidget()));
	// not sure if this a good idea for a docable widget

	// create new debuggable viewer window
	auto* viewer = new BitMapViewer();
	auto* dw = new DockableWidget(dockMan);
	dw->setWidget(viewer);
	dw->setTitle(tr("Bitmapped VRAM View"));
	dw->setId("BITMAPVRAMVIEW");
	dw->setFloating(true);
	dw->setDestroyable(true);
	dw->setMovable(true);
	dw->setClosable(true);

	/*
	connect(dw, &DockableWidget::visibilityChanged,
	        this, &DebuggerForm::dockWidgetVisibilityChanged);
	connect(this, &DebuggerForm::debuggablesChanged,
	        viewer, &BitMapViewer::setDebuggables);
	*/

	// TODO: refresh should be handled by VDPDataStore...
	connect(this, &DebuggerForm::connected, viewer, &BitMapViewer::refresh);
	connect(this, &DebuggerForm::breakStateEntered, viewer, &BitMapViewer::refresh);

	/*
	viewer->setDebuggables(debuggables);
	viewer->setEnabled(disasmView->isEnabled());
	*/
}

void DebuggerForm::toggleCharMappedDisplay()
{
	//toggleView(qobject_cast<DockableWidget*>(slotView->parentWidget()));
	// not sure if this a good idea for a docable widget

	// create new debuggable viewer window
	auto* viewer = new TileViewer();
	//    viewer->adjustSize();
	auto* dw = new DockableWidget(dockMan);
	dw->setWidget(viewer);
	dw->setTitle(tr("Tile VRAM View"));
	dw->setId("CHARMAPVRAMVIEW");
	dw->setFloating(true);
	dw->setDestroyable(true);
	dw->setMovable(true);
	dw->setClosable(true);
	//    dw->adjustSize();

	// TODO: refresh should be handled by VDPDataStore...
	connect(this, &DebuggerForm::connected, viewer, &TileViewer::refresh);
	connect(this, &DebuggerForm::breakStateEntered, viewer, &TileViewer::refresh);
}

void DebuggerForm::toggleSpritesDisplay()
{
	//toggleView(qobject_cast<DockableWidget*>(slotView->parentWidget()));
	// not sure if this a good idea for a docable widget

	// create new debuggable viewer window
	auto* viewer = new SpriteViewer();
	auto* dw = new DockableWidget(dockMan);
	dw->setWidget(viewer);
	dw->setTitle(tr("Sprites View"));
	dw->setId("SPRITESVRAMVIEW");
	dw->setFloating(true);
	dw->setDestroyable(true);
	dw->setMovable(true);
	dw->setClosable(true);

	// TODO: refresh should be handled by VDPDataStore...
	connect(this, &DebuggerForm::breakStateEntered, viewer, &SpriteViewer::refresh);
}

void DebuggerForm::toggleVDPCommandRegsDisplay()
{
	if (VDPCommandRegView == nullptr) {
		VDPCommandRegView = new VDPCommandRegViewer();
		auto* dw = new DockableWidget(dockMan);
		dw->setWidget(VDPCommandRegView);
		dw->setTitle(tr("VDP registers view"));
		dw->setId("VDPCommandRegView");
		dw->setFloating(true);
		dw->setDestroyable(false);
		dw->setMovable(true);
		dw->setClosable(true);
		connect(this, &DebuggerForm::breakStateEntered,
		        VDPCommandRegView, &VDPCommandRegViewer::refresh);
	} else {
		toggleView(qobject_cast<DockableWidget*>(VDPCommandRegView->parentWidget()));
	}
}

void DebuggerForm::toggleVDPRegsDisplay()
{
	if (VDPRegView == nullptr) {
		VDPRegView = new VDPRegViewer();
		auto* dw = new DockableWidget(dockMan);
		dw->setWidget(VDPRegView);
		dw->setTitle(tr("VDP registers view"));
		dw->setId("VDPREGVIEW");
		dw->setFloating(true);
		dw->setDestroyable(false);
		dw->setMovable(true);
		dw->setClosable(true);
		connect(this, &DebuggerForm::breakStateEntered,
		        VDPRegView, &VDPRegViewer::refresh);
	} else {
		toggleView(qobject_cast<DockableWidget*>(VDPRegView->parentWidget()));
	}
}

void DebuggerForm::toggleVDPStatusRegsDisplay()
{
	if (VDPStatusRegView == nullptr) {
		VDPStatusRegView = new VDPStatusRegViewer();
		auto* dw = new DockableWidget(dockMan);
		dw->setWidget(VDPStatusRegView);
		dw->setTitle(tr("VDP status registers view"));
		dw->setId("VDPSTATUSREGVIEW");
		dw->setFloating(true);
		dw->setDestroyable(false);
		dw->setMovable(true);
		dw->setClosable(true);
		connect(this, &DebuggerForm::breakStateEntered,
		        VDPStatusRegView, &VDPStatusRegViewer::refresh);
	} else {
		toggleView(qobject_cast<DockableWidget*>(VDPStatusRegView->parentWidget()));
	}
}

void DebuggerForm::toggleMemoryDisplay()
{
	toggleView(qobject_cast<DockableWidget*>(mainMemoryView->parentWidget()));
}

void DebuggerForm::toggleView(DockableWidget* widget)
{
	if (widget->isHidden()) {
		widget->show();
	} else {
		widget->hide();
	}
	dockMan.visibilityChanged(widget);
}

void DebuggerForm::addDebuggableViewer()
{
	// create new debuggable viewer window
	auto* viewer = new DebuggableViewer();
	auto* dw = new DockableWidget(dockMan);
	dw->setWidget(viewer);
	dw->setTitle(tr("Debuggable hex view"));
	dw->setId("DEBUGVIEW-" + QString::number(++counter));
	dw->setFloating(true);
	dw->setDestroyable(true);
	dw->setMovable(true);
	dw->setClosable(true);
	connect(dw, &DockableWidget::visibilityChanged,
	        this, &DebuggerForm::dockWidgetVisibilityChanged);
	connect(this, &DebuggerForm::debuggablesChanged,
	        viewer, &DebuggableViewer::setDebuggables);
	connect(this, &DebuggerForm::breakStateEntered,
	        viewer, &DebuggableViewer::refresh);
	viewer->setDebuggables(debuggables);
	viewer->setEnabled(disasmView->isEnabled());
}

void DebuggerForm::showFloatingWidget()
{
	QObject * s = sender();
	QString widget_id = s->property("widget_id").toString();
	DockableWidget* w = dockMan.findDockableWidget(widget_id);
	w->show();
	w->activateWindow();
	w->raise();
}

void DebuggerForm::dockWidgetVisibilityChanged(DockableWidget* w)
{
	dockMan.visibilityChanged(w);
	updateViewMenu();
}

void DebuggerForm::updateViewMenu()
{
	viewRegistersAction->setChecked(regsView->parentWidget()->isVisible());
	viewFlagsAction->setChecked(flagsView->isVisible());
	viewStackAction->setChecked(stackView->isVisible());
	viewSlotsAction->setChecked(slotView->isVisible());
	viewMemoryAction->setChecked(mainMemoryView->isVisible());
	viewBreakpointsAction->setChecked(bpView->isVisible());
}

void DebuggerForm::updateVDPViewMenu()
{
	if (VDPCommandRegView) {
		viewVDPCommandRegsAction->setChecked(VDPCommandRegView->isVisible());
	}
	if (VDPRegView) {
		viewVDPRegsAction->setChecked(VDPRegView->isVisible());
	}
	if (VDPStatusRegView) {
		viewVDPStatusRegsAction->setChecked(VDPStatusRegView->isVisible());
	}
}

void DebuggerForm::updateViewFloatingWidgetsMenu()
{
	viewFloatingWidgetsMenu->clear();
	for (auto* w : dockMan.managedWidgets()) {
		if (w->isFloating()) {
			// Build up the window title
			auto* action = new QAction(w->title());
			// Set the widget_id as a property on the menu item action, so we can read it out later again via sender() on the receiving functor
			action->setProperty("widget_id", w->id());
			connect(action, &QAction::triggered, this, &DebuggerForm::showFloatingWidget);
			viewFloatingWidgetsMenu->addAction(action);
		}
	}
}

void DebuggerForm::setDebuggables(const QString& list)
{
	debuggables.clear();

	// process result string
	QStringList l = list.split(" ", Qt::SplitBehaviorFlags::SkipEmptyParts);
	for (int i = 0; i < l.size(); ++i) {
		QString d = l[i];
		// combine multiple words
		if (d[0] == '{') {
			while (!d.endsWith("}")) {
				d.push_back(' ');
				d.append(l[++i]);
			}
		}
		// set initial size to zero
		debuggables[d] = 0;
	}
	// find the size for all debuggables
	for (auto it = debuggables.begin(); it != debuggables.end(); ++it) {
		comm.sendCommand(new DebuggableSizeHandler(it.key(), *this));
	}
}

void DebuggerForm::setDebuggableSize(const QString& debuggable, int size)
{
	debuggables[debuggable] = size;
	// emit update if size of last debuggable was set
	if (debuggable == debuggables.keys().last()) {
		emit debuggablesChanged(debuggables);
	}
}

void DebuggerForm::symbolFileChanged()
{
	static bool shown(false);
	if (shown) return;
	if (!Settings::get().autoReloadSymbols()) {
		shown = true;
		int choice = QMessageBox::question(this, tr("Symbol file changed"),
						tr("One or more symbol file have changed.\n"
						"Reload now?"),
						QMessageBox::Yes | QMessageBox::No);
		shown = false;
		if (choice == QMessageBox::No) return;
	}
	session.symbolTable().reloadFiles();
	emit symbolFilesChanged();
}

DebuggerForm::AddressSlotResult DebuggerForm::addressSlot(int addr) const
{
	int p = (addr & 0xC000) >> 14;
	uint8_t ps = memLayout.primarySlot[p];
	int8_t ss_ = memLayout.secondarySlot[p];
	auto ss = make_positive_optional<uint8_t>(ss_);

	std::optional<uint8_t> segment = [&] { // figure out (rom) mapper segment
		if (ss && memLayout.mapperSize[ps][*ss] > 0) {
			return std::optional(uint8_t(memLayout.mapperSegment[p]));
		} else {
			int q = 2 * p + ((addr & 0x2000) >> 13);
			int b = memLayout.romBlock[q];
			return make_if(b >= 0, uint8_t(b));
		}
	}();
	return {ps, ss, segment};
}

void DebuggerForm::reloadBreakpoints(bool merge)
{
	auto* command = new Command("debug_list_all_breaks",
	                            [this, merge](const QString& message) {
	                                    if (merge) {
	                                            processMerge(message);
	                                    } else {
	                                            processBreakpoints(message);
	                                    }
	                            });
	comm.sendCommand(command);
}

void DebuggerForm::processBreakpoints(const QString& message)
{
	session.breakpoints().setBreakpoints(message);
	disasmView->update();
	session.sessionModified();
	updateWindowTitle();
	emit breakpointsUpdated();
}

void DebuggerForm::processMerge(const QString& message)
{
	QString bps = session.breakpoints().mergeBreakpoints(message);
	if (!bps.isEmpty()) {
		comm.sendCommand(new SimpleCommand(bps));
		reloadBreakpoints(false);
	} else {
		processBreakpoints(message);
	}
}

QByteArray DebuggerForm::saveCommands() const
{
	QByteArray output;

	auto first = true;
	for (const auto& [name, description, source, icon, _] : commands) {
		if (!first) output.append(':');
		first = false;
		output.append(name.toUtf8().toBase64());
		output.append(';');
		output.append(description.toUtf8().toBase64());
		output.append(';');
		output.append(source.toUtf8().toBase64());
		output.append(';');
		output.append(icon.toUtf8().toBase64());
	}

	return output;
}

void DebuggerForm::restoreCommands(const QByteArray& input)
{
	commands.clear();

	auto commandBytes = input.split(':');
	auto counter = 0;
	for (const auto& cb : commandBytes) {
		auto fields = cb.split(';');
		if (fields.count() < 4) continue;

		QString name = QByteArray::fromBase64(fields[0]);
		CommandRef command{
			name,
			QByteArray::fromBase64(fields[1]),
			QByteArray::fromBase64(fields[2]),
			QByteArray::fromBase64(fields[3]),
			counter++
		};
		commands.append(command);
	}
}

void DebuggerForm::onSlotsUpdated(bool slotsChanged)
{
	if (disasmStatus == PC_CHANGED) {
		disasmView->setProgramCounter(disasmAddress, slotsChanged);
		disasmStatus = RESET;
	} else {
		disasmStatus = slotsChanged ? SLOTS_CHANGED : SLOTS_CHECKED;
	}
}

void DebuggerForm::onPCChanged(uint16_t address)
{
	if (disasmStatus != RESET) {
		disasmView->setProgramCounter(address, disasmStatus == SLOTS_CHANGED);
	} else {
		disasmStatus = PC_CHANGED;
		disasmAddress = address;
	}
}
