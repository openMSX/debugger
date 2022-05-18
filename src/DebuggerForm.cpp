#include "DebuggerForm.h"
#include "BitMapViewer.h"
#include "TileViewer.h"
#include "SpriteViewer.h"
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
#include "GotoDialog.h"
#include "DebuggableViewer.h"
#include "VDPRegViewer.h"
#include "VDPStatusRegViewer.h"
#include "VDPCommandRegViewer.h"
#include "Settings.h"
#include "Version.h"
#include "SignalDispatcher.h"
#include "blendsplitter/BlendSplitter.h"
#include "blendsplitter/WidgetRegistry.h"
#include "QuickGuide.h"
#include "PaletteView.h"
#include "TabRenamerHelper.h"
#include <QAction>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QSplitter>
#include <QPixmap>
#include <QFileDialog>
#include <QCloseEvent>
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


QMap<QString, int> DebuggerForm::debuggables;

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
	CPURegRequest(DebuggerForm& /*form_*/)
		: ReadDebugBlockCommand("{CPU regs}", 0, 28, buf)
		//, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		copyData(message);
		SignalDispatcher::instance().setData(buf);
		delete this;
	}

private:
	//DebuggerForm& form;
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
    session = DebugSession::getDebugSession();

	createActions();
	createMenus();
	createToolbars();
	createStatusbar();
	createForm();

	recentFiles = Settings::get().value("MainWindow/RecentFiles").toStringList();
	updateRecentFiles();

    connect(&(session->symbolTable()), &SymbolTable::symbolFileChanged, this, &DebuggerForm::symbolFileChanged);
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
    fileSaveSessionAction->setStatusTip(tr("Save the current debug session"));

	fileSaveSessionAsAction = new QAction(tr("Save Session &As"), this);
	fileSaveSessionAsAction->setStatusTip(tr("Save the debug session in a selected file"));

    fileOpenWorkspaceLayoutAction = new QAction(tr("&Open workspaces"), this);
    fileOpenWorkspaceLayoutAction->setStatusTip(tr("Load a workspace layout."));

    fileSaveWorkspaceLayoutAction = new QAction(tr("&Save workspaces"), this);
    fileSaveWorkspaceLayoutAction->setStatusTip(tr("Save the current workspaces and splitters layout"));

    fileSaveWorkspaceLayoutAsAction = new QAction(tr("Save workspaces &As"), this);
    fileSaveWorkspaceLayoutAsAction->setStatusTip(tr("Save the current workspaces and splitters in a selected file"));




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

	breakpointAddAction = new QAction(tr("Add ..."), this);
    //now that we have possible multiple disasmviewers the disasmview->getCursorAddr() will not work anymore
//	breakpointAddAction->setShortcut(tr("CTRL+B"));
//	breakpointAddAction->setStatusTip(tr("Add a breakpoint at a location"));
//	breakpointAddAction->setEnabled(false);

	helpAboutAction = new QAction(tr("&About"), this);

    addCPUWorkspaceAction = new QAction(tr("&Code debugger"), this);
    addCPUWorkspaceAction->setStatusTip(tr("The default way of debugging CPU code"));

    addVDPRegsWorkspaceAction = new QAction(tr("VDP &Regs"), this);
    addVDPRegsWorkspaceAction->setStatusTip(tr("Show the regular VDP registers"));

    addVDPTilesWorkspaceAction = new QAction(tr("VDP &Tiles"), this);
    addVDPTilesWorkspaceAction->setStatusTip(tr("Show the VRAM in tiles"));

    addVDPBitmapWorkspaceAction = new QAction(tr("VDP &Bitmap"), this);
    addVDPBitmapWorkspaceAction->setStatusTip(tr("Show the VRAM in bitmap mode"));

    addEmptyWorkspaceAction = new QAction(tr("&Empty workspace"), this);
    addEmptyWorkspaceAction->setStatusTip(tr("create an almost empty workspace"));

    addFloatingSwitchingWidgetAction = new QAction(tr("&Create floating item"), this);
    addFloatingSwitchingWidgetAction->setStatusTip(tr("Create item in seperate window"));


	connect(fileNewSessionAction, &QAction::triggered, this, &DebuggerForm::fileNewSession);
	connect(fileOpenSessionAction, &QAction::triggered, this, &DebuggerForm::fileOpenSession);
	connect(fileSaveSessionAction, &QAction::triggered, this, &DebuggerForm::fileSaveSession);
	connect(fileSaveSessionAsAction, &QAction::triggered, this, &DebuggerForm::fileSaveSessionAs);

    connect(fileOpenWorkspaceLayoutAction, &QAction::triggered, this, &DebuggerForm::fileOpenWorkspace);
    connect(fileSaveWorkspaceLayoutAction, &QAction::triggered, this, &DebuggerForm::fileSaveWorkspace);
    connect(fileSaveWorkspaceLayoutAsAction, &QAction::triggered, this, &DebuggerForm::fileSaveWorkspaceAs);

	connect(fileQuitAction, &QAction::triggered, this, &DebuggerForm::close);
	connect(systemConnectAction, &QAction::triggered, this, &DebuggerForm::systemConnect);
	connect(systemDisconnectAction, &QAction::triggered, this, &DebuggerForm::systemDisconnect);
	connect(systemPauseAction, &QAction::triggered, this, &DebuggerForm::systemPause);
	connect(systemRebootAction, &QAction::triggered, this, &DebuggerForm::systemReboot);
	connect(systemSymbolManagerAction, &QAction::triggered, this, &DebuggerForm::systemSymbolManager);
	connect(systemPreferencesAction, &QAction::triggered, this, &DebuggerForm::systemPreferences);
	connect(searchGotoAction, &QAction::triggered, this, &DebuggerForm::searchGoto);

    connect(executeBreakAction, &QAction::triggered, this, &DebuggerForm::executeBreak);
	connect(executeRunAction, &QAction::triggered, this, &DebuggerForm::executeRun);
	connect(executeStepAction, &QAction::triggered, this, &DebuggerForm::executeStep);
	connect(executeStepOverAction, &QAction::triggered, this, &DebuggerForm::executeStepOver);
	connect(executeStepOutAction, &QAction::triggered, this, &DebuggerForm::executeStepOut);
	connect(executeStepBackAction, &QAction::triggered, this, &DebuggerForm::executeStepBack);
	connect(helpAboutAction, &QAction::triggered, this, &DebuggerForm::showAbout);
    connect(addCPUWorkspaceAction, &QAction::triggered, this, &DebuggerForm::addCPUWorkspace);
    connect(addVDPRegsWorkspaceAction, &QAction::triggered, this, &DebuggerForm::addVDPRegsWorkspace);
    connect(addVDPTilesWorkspaceAction, &QAction::triggered, this, &DebuggerForm::addVDPTilesWorkspace);
    connect(addVDPBitmapWorkspaceAction, &QAction::triggered, this, &DebuggerForm::addVDPBitmapWorkspace);
    connect(addEmptyWorkspaceAction, &QAction::triggered, this, &DebuggerForm::addEmptyWorkspace);
    connect(addFloatingSwitchingWidgetAction, &QAction::triggered, this, &DebuggerForm::addFloatingSwitchingWidget);
}

void DebuggerForm::createMenus()
{
	// create file menu
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(fileNewSessionAction);
	fileMenu->addAction(fileOpenSessionAction);
	fileMenu->addAction(fileSaveSessionAction);
	fileMenu->addAction(fileSaveSessionAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(fileOpenWorkspaceLayoutAction);
//    fileMenu->addAction(fileSaveWorkspaceLayoutAction);
    fileMenu->addAction(fileSaveWorkspaceLayoutAsAction);
    fileMenu->addSeparator();

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

	// create execute menu
	executeMenu = menuBar()->addMenu(tr("&Execute"));
	executeMenu->addAction(executeBreakAction);
	executeMenu->addAction(executeRunAction);
	executeMenu->addSeparator();
	executeMenu->addAction(executeStepAction);
	executeMenu->addAction(executeStepOverAction);
	executeMenu->addAction(executeStepOutAction);
	executeMenu->addAction(executeStepBackAction);

	// create breakpoint menu
	breakpointMenu = menuBar()->addMenu(tr("&Breakpoint"));
    //breakpointMenu->addAction(breakpointToggleAction);
	breakpointMenu->addAction(breakpointAddAction);

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
    //executeToolbar->addAction(executeRunToAction);
}

void DebuggerForm::createStatusbar()
{
	// create the statusbar
	statusBar()->showMessage("No emulation running.");
}

void DebuggerForm::createWidgetRegistry()
{
    auto& registry = WidgetRegistry::instance();

    //0: register the disasm viewer widget
    registry.addItem(new RegistryItem{
	tr("Code view"),
	[] { return widgetFactory( disasmViewer); }});

    //1: register the memory view widget
    registry.addItem(new RegistryItem{
	tr("Main memory"),
	[] { return widgetFactory( mainMemoryViewer); }});

    //2: register the register viewer
    registry.addItem(new RegistryItem{
	tr("CPU registers"),
        [] { return widgetFactory(cpuRegsViewer); }});

    //3: register the flags viewer
    registry.addItem(new RegistryItem{
	tr("Flags"),
        [] { return widgetFactory(flagsViewer); }});

    //4: register the stack viewer
    registry.addItem(new RegistryItem{
	tr("Stack"),
        [] { return widgetFactory(stackViewer); }});

    //5: register the slot viewer
    registry.addItem(new RegistryItem{
	tr("Memory layout"),
        [] { return widgetFactory(slotViewer); }});

    //6: register the breakpoints viewer
    registry.addItem(new RegistryItem{
	tr("Debug list"),
        [] { return widgetFactory(breakpointViewer); }});

    //7: register the debuggable viewer
    registry.addItem(new RegistryItem{
	tr("Debuggable hex view"),
        [] { return widgetFactory(debuggableViewer); }});
    //registry.setDefault(item);

    //8: register the VDP Status Registers
    registry.addItem(new RegistryItem{
	tr("VDP status registers"),
        [] { return widgetFactory(vdpStatusRegViewer); }});

    //9: register the VDP command registers view
    registry.addItem(new RegistryItem{
	tr("VDP command registers "),
        [] { return widgetFactory(vdpCommandRegViewer); }});

    //10: register the Bitmapped VRAM View
    registry.addItem(new RegistryItem{
	tr("VRAM as bitmap"),
        [] { return widgetFactory(bitMapViewer); }});

    //11: register the Tile VRAM View
    registry.addItem(new RegistryItem{
	tr("VRAM as tiles"),
        [] { return widgetFactory(tileViewer); }});

    //12: register the Sprites View
    registry.addItem(new RegistryItem{
	tr("Sprites View"),
        [] { return widgetFactory(spriteViewer); }});

    //13: register the general VDP registers
    registry.addItem(new RegistryItem{
        tr("VDP registers"),
        [] { return widgetFactory(vdpRegisters); }});

    //14: register the quick guide manuel
    auto* item  = new RegistryItem{
        tr("Quick Guide"),
        [] { return widgetFactory(quickguide); }};
    registry.addItem(item);
    registry.setDefault(item);

    //15: register the palette editor
    registry.addItem(new RegistryItem{
        tr("Palette editor"),
        [] { return widgetFactory(paletteViewer); }});
}

BlendSplitter* DebuggerForm::createWorkspaceCPU()
{
    auto& registry = WidgetRegistry::instance();

    auto* split = new BlendSplitter(Qt::Horizontal);
    split->addWidget(registry.item(2)); //2: the register viewer
    split->addWidget(registry.item(3)); //3: the flags viewer
    split->addWidget(registry.item(5)); //5: the slot viewer

    auto* split2 = new BlendSplitter(Qt::Vertical);
    split2->addSplitter(split);
    split2->addWidget(registry.item(1)); //1: the memory view widget
    split2->addWidget(registry.item(6)); //6: the breakpoints viewer

    auto* split3 = new BlendSplitter(Qt::Horizontal);
    split3->addWidget(registry.item(0)); //0: the disasm viewer
    split3->addSplitter(split2);
    split3->addWidget(registry.item(4)); //4: the stack viewer

    return split3;
}

BlendSplitter* DebuggerForm::createWorkspaceVDPRegs()
{
    auto& registry = WidgetRegistry::instance();

    auto* split2 = new BlendSplitter(Qt::Vertical);
    split2->addWidget(registry.item(8)); //8: the VDP Status Registers
    split2->addWidget(registry.item(9)); //9: the VDP command registers view

    auto* split3 = new BlendSplitter(Qt::Horizontal);
    split3->addWidget(registry.item(13)); //13: the general VDP registers
    split3->addSplitter(split2);

    return split3;
}

BlendSplitter *DebuggerForm::createWorkspaceVDPTiles()
{
    auto& registry = WidgetRegistry::instance();
    auto* split3 = new BlendSplitter(Qt::Horizontal);
    split3->addWidget(registry.item(11)); //11: the Tile VRAM View
    split3->addWidget(registry.item(12)); //12: the Sprites View
    return split3;
}

BlendSplitter *DebuggerForm::createWorkspaceVDPBitmap()
{
    auto& registry = WidgetRegistry::instance();
    auto* split3 = new BlendSplitter(Qt::Horizontal);
    split3->addWidget(registry.item(10)); //10: the Bitmapped VRAM View
    split3->addWidget(registry.item(12)); //12: the Sprites View
    return split3;
}

void DebuggerForm::tabCloseRequest(int index)
{
    if ((index < 0) || (index >= workspaces->count())) {
        return;
    }
    if (workspaces->count() > 1) {
        QWidget *splitter=workspaces->widget(index);
        workspaces->removeTab(index);
        delete splitter;
    }
}

void DebuggerForm::addInfoWorkspace()
{
    auto* split = new BlendSplitter(Qt::Horizontal);
    split->addWidget(WidgetRegistry::instance().item(14)); //14: the quick guide manuel
    workspaces->addTab(split, "Welcome new user");
}

void DebuggerForm::addCPUWorkspace()
{
    workspaces->addTab(createWorkspaceCPU(), "CPU");
}

void DebuggerForm::addVDPRegsWorkspace()
{
    workspaces->addTab(createWorkspaceVDPRegs(), "VDP Registers");
}

void DebuggerForm::addVDPTilesWorkspace()
{
    workspaces->addTab(createWorkspaceVDPTiles(), "VDP tiles");
}

void DebuggerForm::addVDPBitmapWorkspace(){
    workspaces->addTab(createWorkspaceVDPBitmap(), "VDP bitmap");
}

void DebuggerForm::addEmptyWorkspace()
{
    auto* split = new BlendSplitter(Qt::Horizontal);
    split->addWidget();
    workspaces->addTab(split, "custom");
}

void DebuggerForm::addFloatingSwitchingWidget()
{
    auto& dispatcher = SignalDispatcher::instance();
    auto* wdg = new SwitchingWidget();
    connect(&dispatcher, &SignalDispatcher::enableWidget, wdg, &SwitchingWidget::setEnableWidget);
    wdg->setEnableWidget(dispatcher.getEnableWidget());
    wdg->show();
}

void DebuggerForm::createForm()
{
    updateWindowTitle();

    createWidgetRegistry();

    workspaces = new QTabWidget();
    tabRenamer = new TabRenamerHelper{workspaces};

    workspaces->setMinimumHeight(500);
    workspaces->setTabsClosable(true);
    workspaces->setMovable(true);
    QMenu *workspacemenu=new QMenu();
    QMenu *workspacesubmenu=new QMenu("Predefined layouts");
    workspacesubmenu->addAction(addCPUWorkspaceAction);
    workspacesubmenu->addAction(addVDPRegsWorkspaceAction);
    workspacesubmenu->addAction(addVDPTilesWorkspaceAction);
    workspacesubmenu->addAction(addVDPBitmapWorkspaceAction);
    workspacemenu->addMenu(workspacesubmenu);
    workspacemenu->addSeparator();
    workspacemenu->addAction(addEmptyWorkspaceAction);
    workspacemenu->addSeparator();
    workspacemenu->addAction(addFloatingSwitchingWidgetAction);

    QIcon icon = QIcon::fromTheme(QLatin1String("window-new"));
    QToolButton *btn = new QToolButton();
	btn->setIcon(icon);
    btn->setMenu(workspacemenu);
    btn->setPopupMode(QToolButton::InstantPopup);
    workspaces->setCornerWidget(btn, Qt::TopRightCorner);
    connect(workspaces, &QTabWidget::tabCloseRequested, this, &DebuggerForm::tabCloseRequest);
    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(workspaces);
    window->setLayout(layout);

    Settings& s = Settings::get();

    int workspacetype =s.value("creatingWorkspaceType", 0).toInt();
    switch (workspacetype) {
    case 0:
        addInfoWorkspace();
        [[fallthrough]];
    case 1:
        addDefaultWorkspaces();
        break;
    default:
        if (s.value("creatingWorkspaceFile", "").toString().isEmpty()
            || !loadWorkspaces(s.value("creatingWorkspaceFile", "").toString())) {
            addDefaultWorkspaces();
        }
        break;
    }

    setCentralWidget(window);

    // Have the SignalDispatcher refresh it data for the widgets in the blendsplitter
    auto& dispatcher = SignalDispatcher::instance();
    connect(this, &DebuggerForm::connected, &dispatcher, &SignalDispatcher::refresh);
    connect(this, &DebuggerForm::breakStateEntered, &dispatcher, &SignalDispatcher::refresh);
    //and have it propagate the signals
    connect(this, &DebuggerForm::connected, &dispatcher, &SignalDispatcher::connected);
    connect(this, &DebuggerForm::breakStateEntered, &dispatcher, &SignalDispatcher::breakStateEntered);
    connect(this, &DebuggerForm::debuggablesChanged, &dispatcher, &SignalDispatcher::debuggablesChanged);
    connect(this, &DebuggerForm::breakpointsUpdated, &dispatcher, &SignalDispatcher::breakpointsUpdated);

    // disable all widgets
    connectionClosed();

    connect(&comm, &CommClient::connectionReady, this, &DebuggerForm::initConnection);
    connect(&comm, &CommClient::updateParsed, this, &DebuggerForm::handleUpdate);
    connect(&comm, &CommClient::connectionTerminated, this, &DebuggerForm::connectionClosed);

    session->breakpoints().setMemoryLayout(dispatcher.getMemLayout());
}

void DebuggerForm::addDefaultWorkspaces()
{
    addCPUWorkspace();
    addVDPRegsWorkspace();
    addVDPTilesWorkspace();
    addVDPBitmapWorkspace();
}

QWidget* DebuggerForm::widgetFactory(factoryclasses fctwidget)
{
    auto& dispatcher = SignalDispatcher::instance();

    QWidget* retwdgt;
    switch (fctwidget) {
    case disasmViewer:
    {
        DisasmViewer* wdgt = new DisasmViewer();
        connect(wdgt, &DisasmViewer::breakpointToggled, &dispatcher, &SignalDispatcher::breakpointToggled);
        connect(&dispatcher, &SignalDispatcher::connected, wdgt, &DisasmViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::breakStateEntered, wdgt, &DisasmViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::symbolsChanged, wdgt, &DisasmViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::settingsChanged, wdgt, &DisasmViewer::updateLayout);
        connect(&dispatcher, &SignalDispatcher::setCursorAddress, wdgt, &DisasmViewer::setCursorAddress);
        connect(&dispatcher, &SignalDispatcher::setProgramCounter, wdgt, &DisasmViewer::setProgramCounter);
        connect(&dispatcher, &SignalDispatcher::breakpointsUpdated, wdgt, &DisasmViewer::update);
        wdgt->setMemory(dispatcher.getMainMemory());
        wdgt->setBreakpoints(&DebugSession::getDebugSession()->breakpoints());
        wdgt->setMemoryLayout(dispatcher.getMemLayout());
        wdgt->setSymbolTable(&DebugSession::getDebugSession()->symbolTable());
        retwdgt=wdgt;
    };
        break;
    case mainMemoryViewer:
    {
        MainMemoryViewer* wdgt = new MainMemoryViewer();
        // Main memory viewer
        connect(&dispatcher, &SignalDispatcher::connected,  wdgt, &MainMemoryViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::breakStateEntered, wdgt, &MainMemoryViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::registerChanged, wdgt, &MainMemoryViewer::registerChanged);
        //mainMemoryView->setRegsView(regsView);
        wdgt->setSymbolTable(&DebugSession::getDebugSession()->symbolTable());
        wdgt->setDebuggable("memory", 65536);
        retwdgt=wdgt;
    }
        break;
    case cpuRegsViewer:
    {
        CPURegsViewer* wdgt = new CPURegsViewer();
        //copy current registers to new widget
        for (int id = 0; id < 15; ++id) { // CpuRegs::REG_AF up to CpuRegs::REG_IFF
            static_cast<CPURegsViewer*>(wdgt)->setRegister(id, dispatcher.readRegister(id));
        }
        connect(&dispatcher, &SignalDispatcher::registersUpdate, wdgt, &CPURegsViewer::setData);
        retwdgt=wdgt;
    }
        break;
    case flagsViewer:
    {
        FlagsViewer* wdgt = new FlagsViewer();
        wdgt->setFlags(dispatcher.readRegister(CpuRegs::REG_AF) & 0xFF);
        connect(&dispatcher, &SignalDispatcher::flagsChanged, wdgt, &FlagsViewer::setFlags);
        retwdgt=wdgt;
    }
    break;
    case stackViewer:
    {
        StackViewer* wdgt = new StackViewer();
        wdgt->setData(dispatcher.getMainMemory(), 65536);
        wdgt->setStackPointer(dispatcher.readRegister(CpuRegs::REG_SP));
        connect(&dispatcher, &SignalDispatcher::spChanged, wdgt, &StackViewer::setStackPointer);
        retwdgt=wdgt;
    }
        break;
    case slotViewer:
    {
        SlotViewer* wdgt = new SlotViewer();
        connect(&dispatcher, &SignalDispatcher::connected, wdgt, &SlotViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::breakStateEntered, wdgt, &SlotViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::slotsUpdated, wdgt, &SlotViewer::refresh);
        wdgt->setMemoryLayout(dispatcher.getMemLayout());
        retwdgt=wdgt;
    }
        break;
    case breakpointViewer:
        retwdgt = new BreakpointViewer();
        break;
    case debuggableViewer:
    {
        DebuggableViewer* wdgt = new DebuggableViewer();
        connect(&dispatcher, &SignalDispatcher::breakStateEntered, wdgt, &DebuggableViewer::refresh);
        connect(&dispatcher, &SignalDispatcher::debuggablesChanged, wdgt, &DebuggableViewer::setDebuggables);
        wdgt->setDebuggables(debuggables);
        if (!debuggables.isEmpty()) {
            wdgt->debuggableSelected(0);
            wdgt->refresh();
        }
        retwdgt=wdgt;
    }
        break;
    case vdpStatusRegViewer:
        retwdgt = new VDPStatusRegViewer();
        break;
    case vdpCommandRegViewer:
        retwdgt = new VDPCommandRegViewer();
        break;
    case bitMapViewer:
        retwdgt = new BitMapViewer();
        break;
    case tileViewer:
        retwdgt = new TileViewer();
        break;
    case spriteViewer:
        retwdgt = new SpriteViewer();
        break;
    case vdpRegisters:
        retwdgt = new VDPRegViewer();
        break;
    case quickguide:
        retwdgt = new QuickGuide();
        break;
    case paletteViewer:
    {
        PaletteView* wdgt = new PaletteView();
        connect(&dispatcher, &SignalDispatcher::connected, wdgt, &PaletteView::refresh);
        connect(&dispatcher, &SignalDispatcher::breakStateEntered, wdgt, &PaletteView::refresh);
        retwdgt=wdgt;
    }
        break;
    default:
        retwdgt = new QLabel("Not yet implemented in widgetFactory!");
        break;
    }
    return retwdgt;
}

void DebuggerForm::closeEvent(QCloseEvent* e)
{
	// handle unsaved session
	fileNewSession();
	// cancel if session is still modified
	if (session->isModified()) {
		e->ignore();
		return;
	}

	// store layout
	Settings::get().setValue("Layout/WindowGeometry", saveGeometry());

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
    if (session->existsAsFile()) {
        title = title.arg(session->filename());
	} else {
		title = title.arg("unnamed session");
	}
    if (session->isModified()) {
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
	systemDisconnectAction->setEnabled(false);
	systemConnectAction->setEnabled(true);
	breakpointAddAction->setEnabled(false);

	SignalDispatcher::instance().setEnableWidget(false);
}

void DebuggerForm::finalizeConnection(bool halted)
{
	systemPauseAction->setEnabled(true);
	systemRebootAction->setEnabled(true);
	breakpointAddAction->setEnabled(true);
	// merge breakpoints on connect
	mergeBreakpoints = true;
	if (halted) {
		breakOccurred();
	} else {
		setRunMode();
		updateData();
	}

	emit connected();

	SignalDispatcher::instance().setEnableWidget(true);
}

void DebuggerForm::handleUpdate(const QString& type, const QString& name,
                                const QString& message)
{
	if (type == "status") {
		if (name == "cpu") {
			// running state by default.
			if (message == "suspended") {
				breakOccurred();
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

void DebuggerForm::breakOccurred()
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
    if (session->isModified()) {
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
            if (session->isModified()) return;
		}
	}
    session->clear();
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
    session->open(file);
	if (systemDisconnectAction->isEnabled()) {
		// active connection, merge loaded breakpoints
		reloadBreakpoints(true);
	}
	// update recent
    if (session->existsAsFile()) {
		addRecentFile(file);
	} else {
		removeRecentFile(file);
	}

	updateWindowTitle();
}

void DebuggerForm::fileSaveSession()
{
    if (session->existsAsFile()) {
        session->save();
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
        session->saveAs(d.selectedFiles().at(0));
		// update recent
        if (session->existsAsFile()) {
            addRecentFile(session->filename());
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

void DebuggerForm::fileOpenWorkspace()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open workspace layout"),
        QDir::currentPath(), tr("Debug Workspace Layout Files (*.omdl)"));

    if (!fileName.isEmpty()){
        loadWorkspaces(fileName);
    }
}

void DebuggerForm::fileSaveWorkspace()
{

}

QString DebuggerForm::fileSaveWorkspaceAs()
{
    QFileDialog d(this, tr("Save workspace layout"));
    d.setNameFilter(tr("Debug Workspace Layout Files (*.omdl)"));
    d.setDefaultSuffix("omdl");
    d.setDirectory(QDir::currentPath());
    d.setAcceptMode(QFileDialog::AcceptSave);
    d.setFileMode(QFileDialog::AnyFile);
    QString filename;
    if (d.exec()) {
//        session->saveAs(d.selectedFiles().at(0));
        filename=d.selectedFiles().at(0);
        saveWorkspacesAs(filename);
        // update recent
//        if (session->existsAsFile()) {
//            addRecentFile(session->filename());
//		}
    }
    updateWindowTitle();
    return filename;
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
	SymbolManager symManager(session->symbolTable(), this);
	connect(&symManager, &SymbolManager::symbolTableChanged,
	        session, &DebugSession::sessionModified);
	symManager.exec();
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
	auto& dispatcher = SignalDispatcher::instance();
	GotoDialog gtd(*dispatcher.getMemLayout(), session, this);
	if (gtd.exec()) {
		if (auto addr = gtd.address() ){
				if (addr.has_value()) {
			//disasmView->setCursorAddress(addr, 0, DisasmViewer::MiddleAlways);
			dispatcher.setCursorAddress(addr.value(), 0, DisasmViewer::MiddleAlways);
				}
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

//void DebuggerForm::toggleBreakpointAddress(uint16_t addr)
void DebuggerForm::toggleBreakpoint(uint16_t addr)
{
	QString cmd;
	QString id;
    if (session->breakpoints().isBreakpoint(addr, &id)) {
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

void DebuggerForm::addBreakpoint(uint16_t cursorAddress)
{
	BreakpointDialog bpd(*SignalDispatcher::instance().getMemLayout(), session, this);
	uint16_t addr = cursorAddress;
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

void DebuggerForm::showAbout()
{
	QMessageBox::about(
		this, "openMSX Debugger", QString(Version::full().c_str()));
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
		emit SignalDispatcher::instance().debuggablesChanged(debuggables);
	}
}

void DebuggerForm::symbolFileChanged()
{
	static bool shown(false);
	if (shown) return;
	shown = true;
	int choice = QMessageBox::question(this, tr("Symbol file changed"),
	                tr("One or more symbol file have changed.\n"
	                   "Reload now?"),
	                QMessageBox::Yes | QMessageBox::No);
	shown = false;
	if (choice == QMessageBox::Yes)
        session->symbolTable().reloadFiles();
}

DebuggerForm::AddressSlotResult DebuggerForm::addressSlot(int addr) const
{
	auto& dispatcher = SignalDispatcher::instance();
	int p = (addr & 0xC000) >> 14;
	uint8_t ps = dispatcher.getMemLayout()->primarySlot[p];
	int8_t ss_ = dispatcher.getMemLayout()->secondarySlot[p];
	auto ss = make_positive_optional<uint8_t>(ss_);

	std::optional<uint8_t> segment = [&] { // figure out (rom) mapper segment
		//if (dispatcher.getMemLayout()->mapperSize[ps][ss == -1 ? 0 : ss] > 0) {
		if (ss && dispatcher.getMemLayout()->mapperSize[ps][*ss] > 0) {
			return dispatcher.getMemLayout()->mapperSegment[p];
		} else {
			int q = 2 * p + ((addr & 0x2000) >> 13);
			int b = dispatcher.getMemLayout()->romBlock[q];
			//return make_if(b >= 0, uint8_t(b));
			return (b >= 0) ? b : -1;
		}
	}();
	return {ps, ss, segment};
}

bool DebuggerForm::saveWorkspacesAs(const QString &newFileName)
{
    // open file for save
    QFile file(newFileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(nullptr, tr("Save session ..."),
                             tr("Cannot write file %1:\n%2.")
                              .arg(newFileName, file.errorString()));
        return false;
    };
    QJsonObject jsonObj;
    QJsonArray spacesArray;
    //iterate over workspaces
    for (int i=0;i<workspaces->count();i++){
        QJsonObject jsonTab;
        jsonTab["name"]=workspaces->tabText(i);
        jsonTab["workspace"]=static_cast<BlendSplitter*>(workspaces->widget(i))->save2json();
        spacesArray.append(jsonTab);
    };
    jsonObj["workspaces"]=spacesArray;
    QJsonDocument jsonDoc(jsonObj);
    file.write(jsonDoc.toJson());
    // file.close(); done automatically when going out of scope
    return true;

}

bool DebuggerForm::loadWorkspaces(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(nullptr, tr("Loading workspaces ..."),
                             tr("Cannot read file %1:\n%2.")
                                .arg(filename, file.errorString()));
        return false;
    };
    //Now try to parse the json file
    QJsonParseError parseError;
    QJsonDocument jsonDoc=QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(nullptr, tr("Open workspaces ..."),
                             tr("Parse error at %1:%2.").arg(QString::number(parseError.offset), parseError.errorString())
                             );
        return false;
    };

    // delete all the current tabs before reading the new ones
    while(workspaces->count()) {
        delete workspaces->widget(0);
    };
    //now recreate workspaces
    QJsonObject obj = jsonDoc.object();
    for (const auto& value : obj["workspaces"].toArray()) {
        QJsonObject obj = value.toObject();
        auto* splitter = BlendSplitter::createFromJson(obj["workspace"].toObject());
        workspaces->addTab(splitter, obj["name"].toString());
    }
    return true;
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
    session->breakpoints().setBreakpoints(message);
//    disasmView->update();
    session->sessionModified();
	updateWindowTitle();
	emit breakpointsUpdated();
}

void DebuggerForm::processMerge(const QString& message)
{
    QString bps = session->breakpoints().mergeBreakpoints(message);
	if (!bps.isEmpty()) {
		comm.sendCommand(new SimpleCommand(bps));
		reloadBreakpoints(false);
	} else {
		processBreakpoints(message);
	}
}
