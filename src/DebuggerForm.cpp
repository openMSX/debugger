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
	CPURegRequest(DebuggerForm& form_)
		: ReadDebugBlockCommand("{CPU regs}", 0, 28, buf)
		, form(form_)
	{
	}

	void replyOk(const QString& message) override
	{
		copyData(message);
        SignalDispatcher::getDispatcher()->setData(buf);
		delete this;
	}

private:
	DebuggerForm& form;
	unsigned char buf[28];
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
    , comm(CommClient::instance()),enableWidgetStatus(false)
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
    //0: register the disasm viewer widget
	RegistryItem *item = new RegistryItem{
			tr("Code view"),
            []()->QWidget* {return widgetFactory(disasmViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //1: register the memory view widget
	item  = new RegistryItem{
			tr("Main memory"),
            []()->QWidget* {return widgetFactory(mainMemoryViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //2: register the register viewer
	item  = new RegistryItem{
			tr("CPU registers"),
            []()->QWidget* {return widgetFactory(cpuRegsViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //3: register the flags viewer
	item  = new RegistryItem{
			tr("Flags"),
            []()->QWidget* {return widgetFactory(flagsViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //4: register the stack viewer
	item  = new RegistryItem{
			tr("Stack"),
            []()->QWidget* {return widgetFactory(stackViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //5: register the slot viewer
	item  = new RegistryItem{
			tr("Memory layout"),
            []()->QWidget* {return widgetFactory(slotViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //6: register the breakpoints viewer
	item  = new RegistryItem{
			tr("Debug list"),
            []()->QWidget* {return widgetFactory(breakpointViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //7: register the debuggable viewer
	item  = new RegistryItem{
			tr("Debuggable hex view"),
            []()->QWidget* {return widgetFactory(debuggableViewer);}
			};
    WidgetRegistry::getRegistry()->addItem(item);
    //WidgetRegistry::getRegistry()->setDefault(item);

    //8: register the VDP Status Registers
	item  = new RegistryItem{
			tr("VDP status registers"),
            []()->QWidget* {return widgetFactory(vdpStatusRegViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //9: register the VDP command registers view
	item  = new RegistryItem{
			tr("VDP command registers "),
            []()->QWidget* {return widgetFactory(vdpCommandRegViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //10: register the Bitmapped VRAM View
	item  = new RegistryItem{
			tr("VRAM as bitmap"),
            []()->QWidget* {return widgetFactory(bitMapViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //11: register the Tile VRAM View
	item  = new RegistryItem{
			tr("VRAM as tiles"),
            []()->QWidget* {return widgetFactory(tileViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //12: register the Sprites View
	item  = new RegistryItem{
			tr("Sprites View"),
            []()->QWidget* {return widgetFactory(spriteViewer);}
			};
	WidgetRegistry::getRegistry()->addItem(item);

    //13: register the general VDP registers
    item  = new RegistryItem{
            tr("VDP registers"),
            []()->QWidget* {return widgetFactory(vdpRegisters);}
            };
    WidgetRegistry::getRegistry()->addItem(item);
    //14: register the quick guide manuel
    item  = new RegistryItem{
            tr("Quick Guide"),
            []()->QWidget* {return widgetFactory(quickguide);}
            };
    //WidgetRegistry::getRegistry()->addItem(item);
    WidgetRegistry::getRegistry()->setDefault(item);
}

BlendSplitter* DebuggerForm::createWorkspaceCPU()
{
    BlendSplitter* split = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split->addWidget(WidgetRegistry::getRegistry()->item(2)); //2: the register viewer
    split->addWidget(WidgetRegistry::getRegistry()->item(3)); //3: the flags viewer
    split->addWidget(WidgetRegistry::getRegistry()->item(5)); //5: the slot viewer

    BlendSplitter* split2 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Vertical);
    split2->addSplitter(split);
    split2->addWidget(WidgetRegistry::getRegistry()->item(1)); //1: the memory view widget
    split2->addWidget(WidgetRegistry::getRegistry()->item(6)); //6: the breakpoints viewer

    BlendSplitter* split3 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split3->addWidget(WidgetRegistry::getRegistry()->item(0)); //0: the disasm viewer
    split3->addSplitter(split2);
    split3->addWidget(WidgetRegistry::getRegistry()->item(4)); //4: the stack viewer

    return split3;
}

BlendSplitter* DebuggerForm::createWorkspaceVDPRegs()
{
    BlendSplitter* split2 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Vertical);
    split2->addWidget(WidgetRegistry::getRegistry()->item(8)); //8: the VDP Status Registers
    split2->addWidget(WidgetRegistry::getRegistry()->item(9)); //9: the VDP command registers view

    BlendSplitter* split3 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split3->addWidget(WidgetRegistry::getRegistry()->item(13)); //13: the general VDP registers
    split3->addSplitter(split2);

    return split3;
}

BlendSplitter *DebuggerForm::createWorkspaceVDPTiles()
{
    BlendSplitter* split3 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split3->addWidget(WidgetRegistry::getRegistry()->item(11)); //11: the Tile VRAM View
    split3->addWidget(WidgetRegistry::getRegistry()->item(12)); //12: the Sprites View

    return split3;
}

BlendSplitter *DebuggerForm::createWorkspaceVDPBitmap()
{
    BlendSplitter* split3 = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split3->addWidget(WidgetRegistry::getRegistry()->item(10)); //10: the Bitmapped VRAM View
    split3->addWidget(WidgetRegistry::getRegistry()->item(12)); //12: the Sprites View

    return split3;
}

void DebuggerForm::tabCloseRequest(int index)
{
    if((index < 0) || (index >= workspaces->count())){
        return;
    };
    //index 0 is the CPU workspace, refuse to delete this one for now
    //Also this way at least one workspace remains, altough we could do this diferently in the future
    if (index > 0){
        QWidget *splitter=workspaces->widget(index);
        workspaces->removeTab(index);
        delete splitter;
    }
}

void DebuggerForm::addVDPRegsWorkspace(){
    workspaces->addTab(createWorkspaceVDPRegs(),"VDP Registers");
}
void DebuggerForm::addVDPTilesWorkspace(){
    workspaces->addTab(createWorkspaceVDPTiles(),"VDP tiles");
}
void DebuggerForm::addVDPBitmapWorkspace(){
    workspaces->addTab(createWorkspaceVDPBitmap(),"VDP bitmap");
}

void DebuggerForm::addEmptyWorkspace()
{
    BlendSplitter* split = new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},Qt::Horizontal);
    split->addWidget();
    workspaces->addTab(split,"custom");
}

void DebuggerForm::addFloatingSwitchingWidget()
{
    SwitchingWidget* wdg=new SwitchingWidget();
    connect(SignalDispatcher::getDispatcher(), SIGNAL(enableWidget(bool)), wdg, SLOT(setEnableWidget(bool)));
    wdg->setEnableWidget(enableWidgetStatus);
    wdg->show();
}

void DebuggerForm::createForm()
{
	updateWindowTitle();

	createWidgetRegistry();

    workspaces = new QTabWidget();
    workspaces->setMinimumHeight(500);
    workspaces->setTabsClosable(true);
    QMenu *workspacemenu=new QMenu();
    QMenu *workspacesubmenu=new QMenu("Predefined layouts");
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
    connect(workspaces,&QTabWidget::tabCloseRequested,this,&DebuggerForm::tabCloseRequest);

    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(workspaces);
    window->setLayout(layout);

    workspaces->addTab(createWorkspaceCPU(),"CPU");
    addVDPRegsWorkspace();
    addVDPTilesWorkspace();
    addVDPBitmapWorkspace();


    setCentralWidget(window);

    //have the SignalDispatcher refresh it data for the widgets in the blendsplitter
    connect(this, SIGNAL(connected()), SignalDispatcher::getDispatcher(), SLOT(refresh()));
    connect(this, SIGNAL(breakStateEntered()), SignalDispatcher::getDispatcher(), SLOT(refresh()));
    //and have it propagate the signals
    connect(this, SIGNAL(connected()), SignalDispatcher::getDispatcher(), SIGNAL(connected()));
    connect(this, SIGNAL(breakStateEntered()), SignalDispatcher::getDispatcher(), SIGNAL(breakStateEntered()));
    connect(this, SIGNAL(debuggablesChanged(const QMap<QString,int>&)),
            SignalDispatcher::getDispatcher(), SIGNAL(debuggablesChanged(const QMap<QString,int>&)) );
    connect(this, &DebuggerForm::breakpointsUpdated, SignalDispatcher::getDispatcher(), &SignalDispatcher::breakpointsUpdated);


	// disable all widgets
	connectionClosed();

	connect(&comm, &CommClient::connectionReady, this, &DebuggerForm::initConnection);
	connect(&comm, &CommClient::updateParsed, this, &DebuggerForm::handleUpdate);
	connect(&comm, &CommClient::connectionTerminated, this, &DebuggerForm::connectionClosed);

    session->breakpoints().setMemoryLayout(SignalDispatcher::getDispatcher()->getMemLayout());
}

QWidget *DebuggerForm::widgetFactory(factoryclasses fctwidget)
{
    QWidget* wdgt;
    switch (fctwidget) {
    case disasmViewer:
    {
        wdgt= new DisasmViewer();
        connect(wdgt, SIGNAL(breakpointToggled(uint16_t)), SignalDispatcher::getDispatcher(), SIGNAL(breakpointToggled(uint16_t)));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(connected()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(breakStateEntered()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(symbolsChanged()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(settingsChanged()), wdgt, SLOT(updateLayout()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(setCursorAddress(uint16_t,int,int)), wdgt, SLOT(setCursorAddress(uint16_t,int,int)));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(setProgramCounter(uint16_t,bool)), wdgt, SLOT(setProgramCounter(uint16_t,bool)));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(breakpointsUpdated()), wdgt, SLOT(update()));
        static_cast<DisasmViewer*>(wdgt)->setMemory(SignalDispatcher::getDispatcher()->getMainMemory());
        static_cast<DisasmViewer*>(wdgt)->setBreakpoints(&DebugSession::getDebugSession()->breakpoints());
        static_cast<DisasmViewer*>(wdgt)->setMemoryLayout(SignalDispatcher::getDispatcher()->getMemLayout());
        static_cast<DisasmViewer*>(wdgt)->setSymbolTable(&DebugSession::getDebugSession()->symbolTable());

    };
        break;
    case mainMemoryViewer:
        wdgt = new MainMemoryViewer();
        // Main memory viewer
        connect(SignalDispatcher::getDispatcher(), SIGNAL(connected()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(breakStateEntered()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(registerChanged(int,int)), wdgt, SLOT(registerChanged(int,int)));
        //mainMemoryView->setRegsView(regsView);
        static_cast<MainMemoryViewer*>(wdgt)->setSymbolTable(&DebugSession::getDebugSession()->symbolTable());
        static_cast<MainMemoryViewer*>(wdgt)->setDebuggable("memory", 65536);
        break;
    case cpuRegsViewer:
        wdgt = new CPURegsViewer();
        //copy current registers to new widget
        for(int id=0;id<15;id++){ // CpuRegs::REG_AF up to CpuRegs::REG_IFF
        static_cast<CPURegsViewer*>(wdgt)->setRegister(id,
                                                       SignalDispatcher::getDispatcher()->readRegister(id));
        };
        connect(SignalDispatcher::getDispatcher(), SIGNAL(registersUpdate(unsigned char*)), wdgt, SLOT(setData(unsigned char*)));
        break;
    case flagsViewer:
        wdgt = new FlagsViewer();
        static_cast<FlagsViewer*>(wdgt)->setFlags(SignalDispatcher::getDispatcher()->readRegister(CpuRegs::REG_AF) & 0xFF);
        connect(SignalDispatcher::getDispatcher(), SIGNAL(flagsChanged(quint8)), wdgt, SLOT(setFlags(quint8)));
        break;
    case stackViewer:
        wdgt = new StackViewer();
        static_cast<StackViewer*>(wdgt)->setData(SignalDispatcher::getDispatcher()->getMainMemory(), 65536);
        connect(SignalDispatcher::getDispatcher(), SIGNAL(spChanged(quint16)), wdgt, SLOT(setStackPointer(quint16)));
        break;
    case slotViewer:
        wdgt = new SlotViewer();
        connect(SignalDispatcher::getDispatcher(), SIGNAL(connected()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(breakStateEntered()), wdgt, SLOT(refresh()));
        static_cast<SlotViewer*>(wdgt)->setMemoryLayout(SignalDispatcher::getDispatcher()->getMemLayout());
        connect(SignalDispatcher::getDispatcher(), SIGNAL(updateSlots(const QString&)), wdgt, SLOT(updateSlots(const QString&)));
        // Received status update back from widget after breakStateEntered/connected
        //TODO  has to move to SignalDispatcher just as the register stuff!!!!
//        connect(slotView, &SlotViewer::slotsUpdated, this, &DebuggerForm::onSlotsUpdated);

        break;
    case breakpointViewer:
        wdgt = new BreakpointViewer();
        break;
    case debuggableViewer:
        wdgt = new DebuggableViewer();
        connect(SignalDispatcher::getDispatcher(), SIGNAL(breakStateEntered()), wdgt, SLOT(refresh()));
        connect(SignalDispatcher::getDispatcher(), SIGNAL(debuggablesChanged(const QMap<QString,int>&)),
                wdgt, SLOT(setDebuggables(const QMap<QString,int>&)));
        static_cast<DebuggableViewer*>(wdgt)->setDebuggables(debuggables);
        if (!debuggables.isEmpty()){
            static_cast<DebuggableViewer*>(wdgt)->debuggableSelected(0);
            static_cast<DebuggableViewer*>(wdgt)->refresh();
        };
        break;
    case vdpStatusRegViewer:
        wdgt = new VDPStatusRegViewer();
        break;
    case vdpCommandRegViewer:
        wdgt = new VDPCommandRegViewer();
        break;
    case bitMapViewer:
        wdgt = new BitMapViewer();
        break;
    case tileViewer:
        wdgt = new TileViewer();
        break;
    case spriteViewer:
        wdgt = new SpriteViewer();
        break;
    case vdpRegisters:
        wdgt = new VDPRegViewer();
        break;
    case quickguide:
        wdgt = new QuickGuide();
        break;
    default:
        wdgt = new QLabel("Not yet implemented in widgetFactory!");
        break;
    };
    return wdgt;
}

DebuggerForm::~DebuggerForm()
{
//	delete mainArea;
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

    enableWidgetStatus=false;
    emit SignalDispatcher::getDispatcher()->enableWidget(false);
}

void DebuggerForm::finalizeConnection(bool halted)
{
	systemPauseAction->setEnabled(true);
	systemRebootAction->setEnabled(true);
	breakpointAddAction->setEnabled(true);
	// merge breakpoints on connect
	mergeBreakpoints = true;
	if (halted) {
		breakOccured();
	} else {
		setRunMode();
		updateData();
	}

	emit connected();

    enableWidgetStatus=true;
    emit SignalDispatcher::getDispatcher()->enableWidget(true);
}

void DebuggerForm::handleUpdate(const QString& type, const QString& name,
                                const QString& message)
{
	if (type == "status") {
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
	connect(&symManager, SIGNAL(symbolTableChanged()),
            session, SLOT(sessionModified()));
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
    GotoDialog gtd(*SignalDispatcher::getDispatcher()->getMemLayout(), session, this);
	if (gtd.exec()) {
		int addr = gtd.address();
		if (addr >= 0) {
            //disasmView->setCursorAddress(addr, 0, DisasmViewer::MiddleAlways);
            SignalDispatcher::getDispatcher()->setCursorAddress(addr,0,DisasmViewer::MiddleAlways);
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
		cmd = Breakpoints::createSetCommand(Breakpoint::BREAKPOINT, addr, ps, ss, seg);
	}
	comm.sendCommand(new SimpleCommand(cmd));
	// Get results from command above
	reloadBreakpoints();
}

void DebuggerForm::addBreakpoint(uint16_t cursorAddress)
{
    BreakpointDialog bpd(*SignalDispatcher::getDispatcher()->getMemLayout(), session, this);
    int addr =  cursorAddress;
	auto [ps, ss, seg] = addressSlot(addr);
	bpd.setData(Breakpoint::BREAKPOINT, addr, ps, ss, seg);
	if (bpd.exec()) {
		if (bpd.address() >= 0) {
			QString cmd = Breakpoints::createSetCommand(
				bpd.type(), bpd.address(), bpd.slot(), bpd.subslot(), bpd.segment(),
				bpd.addressEndRange(), bpd.condition());
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
        emit SignalDispatcher::getDispatcher()->debuggablesChanged(debuggables);
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
	int p = (addr & 0xC000) >> 14;
    auto ps = qint8(SignalDispatcher::getDispatcher()->getMemLayout()->primarySlot[p]);
    auto ss = qint8(SignalDispatcher::getDispatcher()->getMemLayout()->secondarySlot[p]);
	int segment = [&] { // figure out (rom) mapper segment
        if (SignalDispatcher::getDispatcher()->getMemLayout()->mapperSize[ps][ss == -1 ? 0 : ss] > 0) {
            return SignalDispatcher::getDispatcher()->getMemLayout()->mapperSegment[p];
		} else {
			int q = 2 * p + ((addr & 0x2000) >> 13);
            int b = SignalDispatcher::getDispatcher()->getMemLayout()->romBlock[q];
			return (b >= 0) ? b : -1;
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

void DebuggerForm::onSlotsUpdated(bool slotsChanged)
{
	if (disasmStatus == PC_CHANGED) {
        //disasmView->setProgramCounter(disasmAddress, slotsChanged);
        SignalDispatcher::getDispatcher()->setProgramCounter(disasmAddress, slotsChanged);
        disasmStatus = RESET;
	} else {
		disasmStatus = slotsChanged ? SLOTS_CHANGED : SLOTS_CHECKED;
	}
}

void DebuggerForm::onPCChanged(uint16_t address)
{
	// PC shouldn't update twice.
	assert(disasmStatus != PC_CHANGED);
	if (disasmStatus != RESET) {
        //disasmView->setProgramCounter(address, disasmStatus == SLOTS_CHANGED);
        SignalDispatcher::getDispatcher()->setProgramCounter(address, disasmStatus == SLOTS_CHANGED);
	} else {
		disasmStatus = PC_CHANGED;
		disasmAddress = address;
	}
}
