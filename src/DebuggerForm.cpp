// $Id$

#include "DebuggerForm.h"
#include "DockableWidgetArea.h"
#include "DockableWidget.h"
#include "DisasmViewer.h"
#include "HexViewer.h"
#include "CPURegsViewer.h"
#include "FlagsViewer.h"
#include "StackViewer.h"
#include "SlotViewer.h"
#include "CommClient.h"
#include "ConnectDialog.h"
#include "SymbolManager.h"
#include "PreferencesDialog.h"
#include "BreakpointDialog.h"
#include "DebuggableViewer.h"
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


class QueryPauseHandler : public SimpleCommand
{
public:
	QueryPauseHandler(DebuggerForm& form_)
		: SimpleCommand("set pause")
		, form(form_)
	{
	}

	virtual void replyOk(const QString& message)
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

	virtual void replyOk(const QString& message)
	{
		form.finalizeConnection(message.trimmed() == "1");
		delete this;
	}
private:
	DebuggerForm& form;
};


class ListBreakPointsHandler : public SimpleCommand
{
public:
	ListBreakPointsHandler(DebuggerForm& form_)
		: SimpleCommand("debug list_bp")
		, form(form_)
	{
	}

	virtual void replyOk(const QString& message)
	{
		form.breakpoints.setBreakpoints(message);
		form.disasmView->update();
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

	virtual void replyOk(const QString& message)
	{
		copyData(message);
		form.regsView->setData(buf);
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

	virtual void replyOk(const QString& message)
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

	virtual void replyOk(const QString& message)
	{
		form.setDebuggableSize(debuggable, message.toInt());
		delete this;
	}
private:
	QString debuggable;
	DebuggerForm& form;
};


DebuggerForm::DebuggerForm(QWidget* parent)
	: QMainWindow(parent)
	, comm(CommClient::instance())
{
	createActions();
	createMenus();
	createToolbars();
	createStatusbar();
	createForm();
}

void DebuggerForm::createActions()
{
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

	systemQuitAction = new QAction(tr("&Quit"), this);
	systemQuitAction->setShortcut(tr("Ctrl+Q"));
	systemQuitAction->setStatusTip(tr("Quit the openMSX debugger"));

	viewRegistersAction = new QAction(tr("CPU &Registers"), this);
	viewRegistersAction->setStatusTip(tr("Toggle the cpu registers display"));
	viewRegistersAction->setCheckable(true);

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

	executeBreakAction = new QAction(tr("Break"), this);
	executeBreakAction->setShortcut(tr("CRTL+B"));
	executeBreakAction->setStatusTip(tr("Halt the execution and enter debug mode"));
	executeBreakAction->setIcon(QIcon(":/icons/break.png"));
	executeBreakAction->setEnabled(false);

	executeRunAction = new QAction(tr("Run"), this);
	executeRunAction->setShortcut(tr("F9"));
	executeRunAction->setStatusTip(tr("Leave debug mode and resume execution"));
	executeRunAction->setIcon(QIcon(":/icons/run.png"));
	executeRunAction->setEnabled(false);

	executeStepAction = new QAction(tr("Step into"), this);
	executeStepAction->setShortcut(tr("F7"));
	executeStepAction->setStatusTip(tr("Execute a single instruction"));
	executeStepAction->setIcon(QIcon(":/icons/stepinto.png"));
	executeStepAction->setEnabled(false);

	executeStepOverAction = new QAction(tr("Step over"), this);
	executeStepOverAction->setShortcut(tr("F8"));
	executeStepOverAction->setStatusTip(tr("Execute the next instruction including any called subroutines"));
	executeStepOverAction->setIcon(QIcon(":/icons/stepover.png"));
	executeStepOverAction->setEnabled(false);

	executeStepOutAction = new QAction(tr("Step out"), this);
	executeStepOutAction->setShortcut(tr("F11"));
	executeStepOutAction->setStatusTip(tr("Resume execution until the current routine has finished"));
	executeStepOutAction->setIcon(QIcon(":/icons/stepout.png"));
	executeStepOutAction->setEnabled(false);

	executeRunToAction = new QAction(tr("Run to"), this);
	executeRunToAction->setShortcut(tr("F4"));
	executeRunToAction->setStatusTip(tr("Resume execution until the selected line is reached"));
	executeRunToAction->setIcon(QIcon(":/icons/runto.png"));
	executeRunToAction->setEnabled(false);

	breakpointToggleAction = new QAction(tr("Toggle"), this);
	breakpointToggleAction->setShortcut(tr("F5"));
	breakpointToggleAction->setStatusTip(tr("Toggle breakpoint on/off at cursor"));
	breakpointToggleAction->setIcon(QIcon(":/icons/break.png"));
	breakpointToggleAction->setEnabled(false);

	breakpointAddAction = new QAction(tr("Add ..."), this);
	breakpointAddAction->setShortcut(tr("CTRL+B"));
	breakpointAddAction->setStatusTip(tr("Add a breakpoint at a location"));
	breakpointAddAction->setEnabled(false);

	helpAboutAction = new QAction(tr("&About"), this);
	executeRunToAction->setStatusTip(tr("Show the appliction information"));

	connect(systemConnectAction, SIGNAL(triggered()), this, SLOT(systemConnect()));
	connect(systemDisconnectAction, SIGNAL(triggered()), this, SLOT(systemDisconnect()));
	connect(systemPauseAction, SIGNAL(triggered()), this, SLOT(systemPause()));
	connect(systemRebootAction, SIGNAL(triggered()), this, SLOT(systemReboot()));
	connect(systemSymbolManagerAction, SIGNAL(triggered()), this, SLOT(systemSymbolManager()));
	connect(systemPreferencesAction, SIGNAL(triggered()), this, SLOT(systemPreferences()));
	connect(systemQuitAction, SIGNAL(triggered()), this, SLOT(close()));
	connect(viewRegistersAction, SIGNAL(triggered()), this, SLOT(toggleRegisterDisplay()));
	connect(viewFlagsAction, SIGNAL(triggered()), this, SLOT(toggleFlagsDisplay()));
	connect(viewStackAction, SIGNAL(triggered()), this, SLOT(toggleStackDisplay()));
	connect(viewSlotsAction, SIGNAL(triggered()), this, SLOT(toggleSlotsDisplay()));
	connect(viewMemoryAction, SIGNAL(triggered()), this, SLOT(toggleMemoryDisplay()));
	connect(viewDebuggableViewerAction, SIGNAL(triggered()), this, SLOT(addDebuggableViewer()));
	connect(executeBreakAction, SIGNAL(triggered()), this, SLOT(executeBreak()));
	connect(executeRunAction, SIGNAL(triggered()), this, SLOT(executeRun()));
	connect(executeStepAction, SIGNAL(triggered()), this, SLOT(executeStep()));
	connect(executeStepOverAction, SIGNAL(triggered()), this, SLOT(executeStepOver()));
	connect(executeRunToAction, SIGNAL(triggered()), this, SLOT(executeRunTo()));
	connect(executeStepOutAction, SIGNAL(triggered()), this, SLOT(executeStepOut()));
	connect(breakpointToggleAction, SIGNAL(triggered()), this, SLOT(breakpointToggle()));
	connect(breakpointAddAction, SIGNAL(triggered()), this, SLOT(breakpointAdd()));
	connect(helpAboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));
}

void DebuggerForm::createMenus()
{
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
	systemMenu->addSeparator();
	systemMenu->addAction(systemQuitAction);

	// create execute menu
	viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(viewRegistersAction);
	viewMenu->addAction(viewFlagsAction);
	viewMenu->addAction(viewStackAction);
	viewMenu->addAction(viewSlotsAction);
	viewMenu->addAction(viewMemoryAction);
	viewMenu->addSeparator();
	viewMenu->addAction(viewDebuggableViewerAction);
	connect( viewMenu, SIGNAL( aboutToShow() ), this, SLOT( updateViewMenu() ) );

	// create execute menu
	executeMenu = menuBar()->addMenu(tr("&Execute"));
	executeMenu->addAction(executeBreakAction);
	executeMenu->addAction(executeRunAction);
	executeMenu->addSeparator();
	executeMenu->addAction(executeStepAction);
	executeMenu->addAction(executeStepOverAction);
	executeMenu->addAction(executeStepOutAction);
	executeMenu->addAction(executeRunToAction);

	// create breakpoint menu
	breakpointMenu = menuBar()->addMenu(tr("&Breakpoint"));
	breakpointMenu->addAction(breakpointToggleAction);
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
	executeToolbar->addAction(executeRunToAction);
}

void DebuggerForm::createStatusbar()
{
	// create the statusbar
	statusBar()->showMessage("No emulation running.");
}

void DebuggerForm::createForm()
{
	setWindowTitle("openMSX Debugger");

	mainArea = new DockableWidgetArea;
	dockMan.addDockArea( mainArea );
	setCentralWidget(mainArea);

	/*
	 * Create main widgets and append them to the list first
	 */
	DockableWidget *dw = new DockableWidget( dockMan );

	// create the disasm viewer widget
	disasmView = new DisasmViewer;
	dw->setWidget(disasmView);
	dw->setTitle(tr("Code view"));
	dw->setId("CODEVIEW");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(false);
	dw->setClosable(false);
	connect( this, SIGNAL( settingsChanged() ),
	         disasmView, SLOT( settingsChanged() ) );
	connect( this, SIGNAL( symbolsChanged() ),
	         disasmView, SLOT( symbolsChanged() ) );
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// create the memory view widget
	hexView = new HexViewer;
	dw = new DockableWidget( dockMan );
	dw->setWidget(hexView);
	dw->setTitle(tr("Main memory"));
	dw->setId("MEMORY");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// create register viewer
	regsView = new CPURegsViewer;
	dw = new DockableWidget( dockMan );
	dw->setWidget(regsView);
	dw->setTitle(tr("CPU registers"));
	dw->setId("REGISTERS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// create flags viewer
	flagsView = new FlagsViewer;
	dw = new DockableWidget( dockMan );
	dw->setWidget(flagsView);
	dw->setTitle(tr("Flags"));
	dw->setId("FLAGS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// create stack viewer
	stackView = new StackViewer;
	dw = new DockableWidget( dockMan );
	dw->setWidget(stackView);
	dw->setTitle(tr("Stack"));
	dw->setId("STACK");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// create slot viewer
	slotView = new SlotViewer;
	dw = new DockableWidget( dockMan );
	dw->setWidget(slotView);
	dw->setTitle(tr("Memory layout"));
	dw->setId("SLOTS");
	dw->setFloating(false);
	dw->setDestroyable(false);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );

	// restore layout
	restoreGeometry( Settings::get().value( "Layout/WindowGeometry", saveGeometry() ).toByteArray() );

	QStringList list = Settings::get().value( "Layout/WidgetLayout" ).toStringList();
	// defaults needed?
	if( !list.size() || !list.at(0).startsWith("CODEVIEW ") ) {
		list.clear();
		list.append( "CODEVIEW D V R 0 -1 -1" );
		list.append( "REGISTERS D V R 0 -1 -1" );
		list.append( "FLAGS D V R 0 -1 -1" );
		int regW = dockMan.findDockableWidget("REGISTERS")->sizeHint().width();
		int regH = dockMan.findDockableWidget("REGISTERS")->sizeHint().height();
		int codeW = dockMan.findDockableWidget("CODEVIEW")->sizeHint().width();
		int codeH = dockMan.findDockableWidget("CODEVIEW")->sizeHint().height();
		int flagW = dockMan.findDockableWidget("FLAGS")->sizeHint().width();
		int slotW = dockMan.findDockableWidget("SLOTS")->sizeHint().width();
		list.append( QString("SLOTS D V R 0 -1 %1").arg(regH) );
		list.append( QString("STACK D V R 0 -1 %1").arg(codeH) );
		list.append( QString("MEMORY D V B %1 %2 %3").arg(codeW)
		                                             .arg(regW + flagW + slotW)
		                                             .arg(codeH - regH) );
	}

	// add widgets
	for( int i = 0; i < list.size(); i++ ) {
		QStringList s = list.at(i).split(" ", QString::SkipEmptyParts);
		// get widget
		if( (dw = dockMan.findDockableWidget(s.at(0))) ) {
			if( s.at(1) == "D" ) {
				// dock widget
				DockableWidgetLayout::DockSide side;
				if( s.at(3) == "T" )
					side = DockableWidgetLayout::TOP;
				else if( s.at(3) == "L" )
					side = DockableWidgetLayout::LEFT;
				else if( s.at(3) == "R" )
					side = DockableWidgetLayout::RIGHT;
				else
					side = DockableWidgetLayout::BOTTOM;
				dockMan.insertWidget( dw, 0, side, s.at(4).toInt(), s.at(5).toInt(), s.at(6).toInt() );
				if( s.at(2) == "H" ) dw->hide();
			} else if( s.at(1) == "F" ) {
				//  float widget
				dw->setFloating(true, s.at(2) == "V");
				dw->resize(  s.at(5).toInt(), s.at(6).toInt() );
				dw->move(  s.at(3).toInt(), s.at(4).toInt() );
			}
		}
	}

	// disable all widgets
	connectionClosed();

	connect(regsView,   SIGNAL(pcChanged(quint16)),
	        disasmView, SLOT(setProgramCounter(quint16)));
	connect(regsView,   SIGNAL(flagsChanged(quint8)),
	        flagsView,  SLOT(setFlags(quint8)));
	connect(regsView,   SIGNAL(spChanged(quint16)),
	        stackView,  SLOT(setStackPointer(quint16)));
	connect(disasmView, SIGNAL(toggleBreakpoint(int)),
	                    SLOT(breakpointToggle(int)));

	connect(&comm, SIGNAL(connectionReady()),
	        SLOT(initConnection()));
	connect(&comm, SIGNAL(updateParsed(const QString&, const QString&, const QString&)),
	        SLOT(handleUpdate(const QString&, const QString&, const QString&)));
	connect(&comm, SIGNAL(connectionTerminated()),
	        SLOT(connectionClosed()));

	// init main memory
	// added four bytes as runover buffer for dasm
	// otherwise dasm would need to check the buffer end continously.
	breakpoints.setMemoryLayout(&memLayout);
	mainMemory = new unsigned char[65536 + 4];
	memset(mainMemory, 0, 65536 + 4);
	disasmView->setMemory(mainMemory);
	disasmView->setBreakpoints(&breakpoints);
	disasmView->setMemoryLayout(&memLayout);
	disasmView->setSymbolTable(&symTable);
	hexView->setDebuggable("memory", 65536);
	stackView->setData(mainMemory, 65536);
	slotView->setMemoryLayout(&memLayout);
}

DebuggerForm::~DebuggerForm()
{
	delete[] mainMemory;
}

void DebuggerForm::closeEvent( QCloseEvent *e )
{
	// store layout
	Settings::get().setValue( "Layout/WindowGeometry", saveGeometry() );

	QStringList layoutList;
	// fill layout list with docked widgets
	dockMan.getConfig( 0, layoutList );
	// append floating widgets
	QList<DockableWidget*>::const_iterator it = dockMan.managedWidgets().begin();
	while( it != dockMan.managedWidgets().end() ) {
		if( (*it)->isFloating() ) {
			QString s("%1 F %2 %3 %4 %5 %6");
			s = s.arg( (*it)->id() );
			if( (*it)->isHidden() )
				s = s.arg("H");
			else
				s = s.arg("V");
			s = s.arg( (*it)->x() ).arg( (*it)->y() )
			     .arg( (*it)->width() ).arg( (*it)->height() );
			layoutList.append( s );
		}
		it++;
	}
	Settings::get().setValue( "Layout/WidgetLayout", layoutList );

	QMainWindow::closeEvent(e);
}

void DebuggerForm::initConnection()
{
	systemConnectAction->setEnabled(false);
	systemDisconnectAction->setEnabled(true);

	comm.sendCommand(new QueryPauseHandler(*this));
	comm.sendCommand(new QueryBreakedHandler(*this));

	comm.sendCommand(new SimpleCommand("update enable status"));

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
	executeRunToAction->setEnabled(false);
	systemDisconnectAction->setEnabled(false);
	systemConnectAction->setEnabled(true);

	QList<DockableWidget*>::const_iterator it = dockMan.managedWidgets().begin();
	while( it != dockMan.managedWidgets().end() ) {
		(*it)->widget()->setEnabled(false);
		it++;
	}
}

void DebuggerForm::finalizeConnection(bool halted)
{
	systemPauseAction->setEnabled(true);
	systemRebootAction->setEnabled(true);
	if (halted) {
		setBreakMode();
		breakOccured();
	} else {
		setRunMode();
		updateData();
	}

	QList<DockableWidget*>::const_iterator it = dockMan.managedWidgets().begin();
	while( it != dockMan.managedWidgets().end() ) {
		(*it)->widget()->setEnabled(true);
		it++;
	}
}

void DebuggerForm::handleUpdate(const QString& type, const QString& name,
                                const QString& message)
{
	if (type == "status") {
		if (name == "cpu") {
			if (message == "suspended") {
				breakOccured();
			} else {
				setRunMode();
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
	setBreakMode();
	updateData();
}

void DebuggerForm::updateData()
{
	comm.sendCommand(new ListBreakPointsHandler(*this));

	// update registers
	// note that a register update is processed, a signal is sent to other
	// widgets as well. Any dependent updates shoud be called before this one.
	CPURegRequest* regs = new CPURegRequest(*this);
	comm.sendCommand(regs);

	// refresh memory viewer
	hexView->refresh();

	// refresh slot viewer
	slotView->refresh();

	emit emulationChanged();
}

void DebuggerForm::setBreakMode()
{
	executeBreakAction->setEnabled(false);
	executeRunAction->setEnabled(true);
	executeStepAction->setEnabled(true);
	executeStepOverAction->setEnabled(true);
	executeStepOutAction->setEnabled(true);
	executeRunToAction->setEnabled(true);
	breakpointToggleAction->setEnabled(true);
	breakpointAddAction->setEnabled(true);
}

void DebuggerForm::setRunMode()
{
	executeBreakAction->setEnabled(true);
	executeRunAction->setEnabled(false);
	executeStepAction->setEnabled(false);
	executeStepOverAction->setEnabled(false);
	executeStepOutAction->setEnabled(false);
	executeRunToAction->setEnabled(false);
	breakpointToggleAction->setEnabled(false);
	breakpointAddAction->setEnabled(false);
}

void DebuggerForm::systemConnect()
{
	OpenMSXConnection* connection = ConnectDialog::getConnection(this);
	if (connection) {
		comm.connectToOpenMSX(connection);
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
	if( systemPauseAction->isChecked() )
		systemPauseAction->trigger();
	if( executeRunAction->isEnabled() )
		executeRun();
	comm.sendCommand(new SimpleCommand("reset"));
}

void DebuggerForm::systemSymbolManager()
{
	SymbolManager symManager( symTable, this );
	symManager.exec();
	emit symbolsChanged();
}

void DebuggerForm::systemPreferences()
{
	PreferencesDialog prefs( this );
	prefs.exec();
	emit settingsChanged();
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
	comm.sendCommand(new SimpleCommand("step_over"));
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
	// TODO
}

void DebuggerForm::breakpointToggle(int addr)
{
	// toggle address unspecified, use cursor address
	if (addr < 0) addr = disasmView->cursorAddress();

	QString cmd;
	if (breakpoints.isBreakpoint(addr)) {
		cmd = "debug remove_bp " + breakpoints.idString(addr);
	} else {
		int p = (addr & 0xC000) >> 14;
		cmd.sprintf("debug set_bp %i { [ pc_in_slot %c %c %i ] }",
		            addr, memLayout.primarySlot[p],
		            memLayout.secondarySlot[p],
		            memLayout.mapperSegment[p]);
	}
	comm.sendCommand(new SimpleCommand(cmd));

	comm.sendCommand(new ListBreakPointsHandler(*this));
}

void DebuggerForm::breakpointAdd()
{
	BreakpointDialog bpd( memLayout, this );
	if( bpd.exec() ) {
		if( bpd.address() > 0 ) {
			QString cmd("debug set_bp %1 { [ pc_in_slot %2 %3 %4 ] }");
			cmd = cmd.arg( bpd.address() );

			if( bpd.slot() == -1 )
				cmd = cmd.arg('X');
			else
				cmd = cmd.arg(bpd.slot());

			if( bpd.subslot() == -1 )
				cmd = cmd.arg('X');
			else
				cmd = cmd.arg(bpd.subslot());

			if( bpd.segment() == -1 )
				cmd = cmd.arg('X');
			else
				cmd = cmd.arg(bpd.segment());

			comm.sendCommand(new SimpleCommand(cmd));

			comm.sendCommand(new ListBreakPointsHandler(*this));
		}
	};
}

void DebuggerForm::showAbout()
{
	QMessageBox::about(
		this, "openMSX Debugger", QString(Version::FULL_VERSION.c_str())
		);
}

void DebuggerForm::toggleRegisterDisplay()
{
	toggleView( qobject_cast<DockableWidget*>(regsView->parentWidget()));
}

void DebuggerForm::toggleFlagsDisplay()
{
	toggleView( qobject_cast<DockableWidget*>(flagsView->parentWidget()));
}

void DebuggerForm::toggleStackDisplay()
{
	toggleView( qobject_cast<DockableWidget*>(stackView->parentWidget()));
}

void DebuggerForm::toggleSlotsDisplay()
{
	toggleView( qobject_cast<DockableWidget*>(slotView->parentWidget()));
}

void DebuggerForm::toggleMemoryDisplay()
{
	toggleView( qobject_cast<DockableWidget*>(hexView->parentWidget()));
}

void DebuggerForm::toggleView( DockableWidget* widget )
{
	if( widget->isHidden() ) {
		widget->show();
	} else {
		widget->hide();
	}
	dockMan.visibilityChanged( widget );
}

void DebuggerForm::addDebuggableViewer()
{
	// create new debuggable viewer window
	DebuggableViewer *viewer = new DebuggableViewer;
	DockableWidget *dw = new DockableWidget( dockMan );
	dw->setWidget(viewer);
	dw->setTitle(tr("Debuggable hex view"));
	dw->setId("DEBUGVIEW");
	dw->setFloating(true);
	dw->setDestroyable(true);
	dw->setMovable(true);
	dw->setClosable(true);
	connect( dw, SIGNAL( visibilityChanged(DockableWidget*) ),
	         this, SLOT( dockWidgetVisibilityChanged(DockableWidget*) ) );
	connect( this, SIGNAL( debuggablesChanged(const QMap<QString,int>&) ),
	         viewer, SLOT( setDebuggables(const QMap<QString,int>&) ) );
	connect( this, SIGNAL( emulationChanged() ),
	         viewer, SLOT( refresh() ) );
	viewer->setDebuggables( debuggables );
	viewer->setEnabled( disasmView->isEnabled() );
}

void DebuggerForm::dockWidgetVisibilityChanged( DockableWidget *w )
{
	dockMan.visibilityChanged( w );
	updateViewMenu();
}

void DebuggerForm::updateViewMenu()
{
	viewRegistersAction->setChecked( regsView->parentWidget()->isVisible() );
	viewFlagsAction->setChecked( flagsView->isVisible() );
	viewStackAction->setChecked( stackView->isVisible() );
	viewSlotsAction->setChecked( slotView->isVisible() );
	viewMemoryAction->setChecked( hexView->isVisible() );
}

void DebuggerForm::setDebuggables( const QString& list )
{
	debuggables.clear();

	// process result string
	QStringList l = list.split(" ", QString::SkipEmptyParts);
	QString d;
	for( int i = 0; i < l.size(); i++ ) {
		d = l[i];
		// combine multiple words
		if( d[0] == '{' ) {
			while( !d.endsWith("}") ) {
				d.push_back(' ');
				d.append( l[++i] );
			}
		}
		// set initial size to zero
		debuggables[d] = 0;
	}
	// find the size for all debuggables
	QMap<QString,int>::iterator it = debuggables.begin();
	while( it != debuggables.end() ) {
		comm.sendCommand(new DebuggableSizeHandler(it.key(), *this));
		it++;
	}
}

void DebuggerForm::setDebuggableSize(  const QString& debuggable, int size )
{
	debuggables[debuggable] = size;
	// emit update if size of last debuggable was set
	if( debuggable == debuggables.keys().last() )
		emit debuggablesChanged( debuggables );
}
