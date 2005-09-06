// $Id$

#include "DebuggerForm.h"

#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QPixmap>

#include "version.h"

DebuggerForm::DebuggerForm( QWidget* parent)
	: QMainWindow( parent )
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
	systemDisconnectAction->setEnabled(FALSE);
	
	systemPauseAction = new QAction(tr("&Pause emulator"), this);
	systemPauseAction->setShortcut(Qt::Key_Pause);
	systemPauseAction->setStatusTip(tr("Pause the emulation"));
	systemPauseAction->setIcon(QIcon(":/icons/pause.png"));
	systemPauseAction->setCheckable(TRUE);
	systemPauseAction->setEnabled(FALSE);
	
	systemExitAction = new QAction(tr("E&xit"), this);
	systemExitAction->setShortcut(tr("Alt+X"));
	systemExitAction->setStatusTip(tr("Quit the openMSX debugger"));
	
	executeBreakAction = new QAction(tr("Break"), this);
	executeBreakAction->setShortcut(tr("CRTL+B"));
	executeBreakAction->setStatusTip(tr("Halt the execution and enter debug mode"));
	executeBreakAction->setIcon(QIcon(":/icons/break.png"));
	executeBreakAction->setEnabled(FALSE);
	
	executeRunAction = new QAction(tr("Run"), this);
	executeRunAction->setShortcut(tr("F9"));
	executeRunAction->setStatusTip(tr("Leave debug mode and resume execution"));
	executeRunAction->setIcon(QIcon(":/icons/run.png"));
	executeRunAction->setEnabled(FALSE);

	executeStepAction = new QAction(tr("Step into"), this);
	executeStepAction->setShortcut(tr("F7"));
	executeStepAction->setStatusTip(tr("Execute a single instruction"));
	executeStepAction->setIcon(QIcon(":/icons/stepinto.png"));
	executeStepAction->setEnabled(FALSE);

	executeStepOverAction = new QAction(tr("Step over"), this);
	executeStepOverAction->setShortcut(tr("F8"));
	executeStepOverAction->setStatusTip(tr("Execute the next instruction including any called subroutines"));
	executeStepOverAction->setIcon(QIcon(":/icons/stepover.png"));
	executeStepOverAction->setEnabled(FALSE);

	executeStepOutAction = new QAction(tr("Step out"), this);
	executeStepOutAction->setShortcut(tr("F11"));
	executeStepOutAction->setStatusTip(tr("Resume execution until the current routine has finished"));
	executeStepOutAction->setIcon(QIcon(":/icons/stepout.png"));
	executeStepOutAction->setEnabled(FALSE);

	executeRunToAction = new QAction(tr("Run to"), this);
	executeRunToAction->setShortcut(tr("F4"));
	executeRunToAction->setStatusTip(tr("Resume execution until the selected line is reached"));
	executeRunToAction->setIcon(QIcon(":/icons/runto.png"));
	executeRunToAction->setEnabled(FALSE);

	helpAboutAction = new QAction(tr("&About"), this);
	executeRunToAction->setStatusTip(tr("Show the appliction information"));

	connect( systemConnectAction, SIGNAL( triggered() ), this, SLOT( systemConnect() ) );
	connect( systemDisconnectAction, SIGNAL( triggered() ), this, SLOT( systemDisconnect() ) );
	connect( systemPauseAction, SIGNAL( triggered() ), this, SLOT( systemPause() ) );
	connect( systemExitAction, SIGNAL( triggered() ), this, SLOT( close() ) );
	connect( executeBreakAction, SIGNAL( triggered() ), this, SLOT( executeBreak() ) );
	connect( executeRunAction, SIGNAL( triggered() ), this, SLOT( executeRun() ) );
	connect( executeStepAction, SIGNAL( triggered() ), this, SLOT( executeStep() ) );
	connect( executeStepOverAction, SIGNAL( triggered() ), this, SLOT( executeStepOver() ) );
	connect( executeRunToAction, SIGNAL( triggered() ), this, SLOT( executeRunTo() ) );
	connect( executeStepOutAction, SIGNAL( triggered() ), this, SLOT( executeStepOut() ) );
	connect( helpAboutAction, SIGNAL( triggered() ), this, SLOT( showAbout() ) );
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
	systemMenu->addAction(systemExitAction);

	// create execute menu
	executeMenu = menuBar()->addMenu(tr("&Execute"));
	executeMenu->addAction(executeBreakAction);
	executeMenu->addAction(executeRunAction);
	executeMenu->addSeparator();
	executeMenu->addAction(executeStepAction);
	executeMenu->addAction(executeStepOverAction);
	executeMenu->addAction(executeStepOutAction);
	executeMenu->addAction(executeRunToAction);

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

QWidget *DebuggerForm::createNamedWidget(const QString& name, QWidget *widget)
{
	QVBoxLayout *vboxLayout = new QVBoxLayout;
	vboxLayout->setMargin(3);
	vboxLayout->setSpacing(2);
	
	QLabel *lbl = new QLabel(name);

	vboxLayout->addWidget(lbl, 0);
	vboxLayout->addWidget(widget, 1);
	
	QWidget *combined = new QWidget;
	combined->setLayout(vboxLayout);

	return combined;
}

void DebuggerForm::createForm()	
{
	mainSplitter = new QSplitter(Qt::Horizontal, this);
	setCentralWidget(mainSplitter);

	// * create the left side of the gui

	disasmSplitter = new QSplitter(Qt::Vertical, this);
	mainSplitter->addWidget(disasmSplitter);
	
	// create the disasm viewer widget
	disasmView = new DisasmViewer;
	disasmSplitter->addWidget(createNamedWidget(tr("Code view:"), disasmView));
	
	// create the memory view widget
	hexView = new HexViewer;
	disasmSplitter->addWidget(createNamedWidget(tr("Main memory:"), hexView));
	
	// * create the right side of the gui
	QWidget *w = new QWidget;
	mainSplitter->addWidget(w);
	QVBoxLayout *rightLayout = new QVBoxLayout;
	rightLayout->setMargin(0);
	rightLayout->setSpacing(0);
	w->setLayout(rightLayout);

	QWidget *w2 = new QWidget;
	rightLayout->addWidget(w2, 0);	
	QHBoxLayout *topLayout = new QHBoxLayout;
	topLayout->setMargin(0);
	topLayout->setSpacing(0);
	w2->setLayout(topLayout);
	
	// create register viewer
	regsView = new CPURegsViewer;
	topLayout->addWidget(createNamedWidget(tr("CPU registers:"), regsView), 0);
	
	// create flags viewer
	flagsView = new FlagsViewer;
	topLayout->addWidget(createNamedWidget(tr("Flags:"), flagsView), 0);

	// create stack viewer
	stackView = new StackViewer;
	topLayout->addWidget(createNamedWidget(tr("Stack:"), stackView), 0);

	// create spacer on the right
	w2 = new QWidget;
	topLayout->addWidget(w2, 1);

	// create rest
	w2 = new QWidget;
	rightLayout->addWidget(w2, 1);
	

	// init main memory
	// added four bytes as runover buffer for dasm
	// otherwise dasm would need to check the buffer end continously.
	mainMemory = new unsigned char[65536 + 4];
	memset(mainMemory, 0, 65536 + 4);
	disasmView->setMemory(mainMemory);
	disasmView->setBreakpoints(&breakpoints);
	hexView->setData("memory", mainMemory, 65536);
	stackView->setData(mainMemory, 65536);
	
	disasmView->setEnabled(FALSE);
	hexView->setEnabled(FALSE);
	regsView->setEnabled(FALSE);
	flagsView->setEnabled(FALSE);
	stackView->setEnabled(FALSE);

	connect(regsView,   SIGNAL( pcChanged(quint16) ),
	        disasmView, SLOT( setProgramCounter(quint16) ) );
	connect(regsView,   SIGNAL( flagsChanged(quint8) ),
	        flagsView,  SLOT( setFlags(quint8) ) );
	connect(regsView,   SIGNAL( spChanged(quint16) ),
	        stackView,  SLOT( setStackPointer(quint16) ) );	

	connect(&comm, SIGNAL( connectionReady() ),
	               SLOT( initConnection() ) );
	connect(&comm, SIGNAL( dataTransferReady(CommRequest *) ),
	               SLOT( dataTransfered(CommRequest *) ) );
	connect(&comm, SIGNAL( dataTransferCancelled(CommRequest *) ),
	               SLOT( cancelTransfer(CommRequest *) ) );
	connect(&comm, SIGNAL( updateReceived(UpdateMessage *) ),
	               SLOT( handleUpdate(UpdateMessage *) ) );
	connect(&comm, SIGNAL( connectionTerminated() ),
	               SLOT( connectionClosed() ) );
	connect(&comm, SIGNAL( errorOccured( CommClient::ConnectionError ) ),
	               SLOT( handleError( CommClient::ConnectionError ) ) );

	connect(disasmView, SIGNAL( needUpdate(CommDebuggableRequest *) ), 
	        &comm,      SLOT( getDebuggableData(CommDebuggableRequest *) ) );
	connect(hexView, SIGNAL( needUpdate(CommDebuggableRequest *) ), 
	        &comm,   SLOT( getDebuggableData(CommDebuggableRequest *) ) );
	connect(stackView, SIGNAL( needUpdate(CommDebuggableRequest *) ), 
	        &comm,     SLOT( getDebuggableData(CommDebuggableRequest *) ) );

}

DebuggerForm::~DebuggerForm()
{
	delete [] mainMemory;
}

void DebuggerForm::initConnection()
{
	CommCommandRequest *reqPause = new CommCommandRequest(INIT_PAUSE, "set pause");
	comm.getCommandResult(reqPause);
	CommCommandRequest *reqBreak = new CommCommandRequest(INIT_BREAK, "debug breaked");
	comm.getCommandResult(reqBreak);
	CommCommandRequest *reqUpdateStatus = new CommCommandRequest(DISCARD_RESULT_ID, "update enable status");
	comm.getCommandResult(reqUpdateStatus);
	CommCommandRequest *reqUpdateBreak = new CommCommandRequest(DISCARD_RESULT_ID, "update enable break");
	comm.getCommandResult(reqUpdateBreak);
	
	// define 'debug_bin2hex' proc for internal use
	CommCommandRequest* bin2hex = new CommCommandRequest(DISCARD_RESULT_ID,
		"proc debug_bin2hex { input } {\n"
		"  set result \"\"\n"
		"  foreach i [split $input {}] {\n"
		"    append result [format %02X [scan $i %c]] \"\"\n"
		"  }\n"
		"  return $result\n"
		"}\n");
	comm.getCommandResult(bin2hex);
}

void DebuggerForm::connectionClosed()
{
	systemDisconnectAction->setEnabled(FALSE);
	systemPauseAction->setEnabled(FALSE);
	executeBreakAction->setEnabled(FALSE);
	executeRunAction->setEnabled(FALSE);
	executeStepAction->setEnabled(FALSE);
	executeStepOverAction->setEnabled(FALSE);
	executeStepOutAction->setEnabled(FALSE);
	executeRunToAction->setEnabled(FALSE);
	systemConnectAction->setEnabled(TRUE);

	disasmView->setEnabled(FALSE);
	hexView->setEnabled(FALSE);
	regsView->setEnabled(FALSE);
	flagsView->setEnabled(FALSE);
	stackView->setEnabled(FALSE);
}

void DebuggerForm::handleError( CommClient::ConnectionError error )
{
	QString msg;
	
	switch(error) {
		case CommClient::CONNECTION_REFUSED:
			msg = tr("A connection could not be established. Make sure openMSX is "
			         "started and listening on \"localhost:9938\".");
			break;
		case CommClient::CONNECTION_CLOSED:
			msg = tr("The connection was closed prematurely.");
			break;
		case CommClient::HOST_ADDRESS_NOT_FOUND:
			msg = tr("The hostname could not be found. Please change the address"
			         " and try again.");
			break;
		case CommClient::UNSPECIFIED_SOCKET_ERROR:
			msg = tr("A socket error occurred. This should not happen! If this "
			         "problem can be reproduced, please submit a bug report "
			         "describing the step to get this error.");
			break;
		case CommClient::NETWORK_ERROR:
			msg = tr("A network error occurred. This usually means that the "
			         "connection to openMSX was interrupted by a network "
			         "problem.");
			break;
		case CommClient::UNKNOWN_ERROR:
			msg = tr("An unknown error occurred. This should not happen! If this "
			         "problem can be reproduced, please submit a bug report "
			         "describing the step to get this error.");
			break;
	}
	QMessageBox::critical(this, "Connection error!", msg, QMessageBox::Ok, QMessageBox::NoButton);
	comm.closeConnection();
	connectionClosed();
}

void DebuggerForm::dataTransfered(CommRequest *r) 
{
	switch(r->sourceID) {
		case DISCARD_RESULT_ID:
			delete r;
			break;
		case DISASM_MEMORY_REQ_ID:
			disasmView->memoryUpdated( (CommMemoryRequest *)r );
			break;
		case CPUREG_REQ_ID:
		{
			CommDebuggableRequest *dr = (CommDebuggableRequest *)r;
			regsView->setData( dr->target );
			delete dr->target;
			delete dr;
			break;
		}
		case HEXDATA_MEMORY_REQ_ID:
			hexView->hexdataTransfered( (CommDebuggableRequest *)r );
			break;
		case STACK_MEMORY_REQ_ID:
			stackView->memdataTransfered( (CommDebuggableRequest *)r );
			break;
		case INIT_PAUSE:
			systemPauseAction->setChecked( ((CommCommandRequest *)r)->result.trimmed()=="on" );
			delete r;
			break;
		case INIT_BREAK:
			finalizeConnection( ((CommCommandRequest *)r)->result.trimmed()=="1" );
			delete r;
			break;
		case BREAKPOINTS_REQ_ID:
			breakpoints.setBreakpoints( ((CommCommandRequest *)r)->result );
			delete r;
			break;
	}
}

void DebuggerForm::finalizeConnection(bool halted)
{
	systemDisconnectAction->setEnabled(TRUE);
	systemPauseAction->setEnabled(TRUE);
	if(halted){
		setBreakMode();
		breakOccured(0);
	}else
		setRunMode();
	disasmView->setEnabled(TRUE);
	hexView->setEnabled(TRUE);
	regsView->setEnabled(TRUE);
	flagsView->setEnabled(TRUE);
	stackView->setEnabled(TRUE);
}	

void DebuggerForm::cancelTransfer(CommRequest *r) 
{
	switch(r->sourceID) {
		case DISASM_MEMORY_REQ_ID:
			disasmView->updateCancelled( (CommMemoryRequest *)r );
			break;
		case CPUREG_REQ_ID:
			delete ((CommDebuggableRequest *)r)->target;
			delete r;
			break;
		case HEXDATA_MEMORY_REQ_ID:
			hexView->transferCancelled( (CommDebuggableRequest *)r );
			break;
		case STACK_MEMORY_REQ_ID:
			stackView->transferCancelled( (CommDebuggableRequest *)r );
			break;
		case INIT_PAUSE:
		case INIT_BREAK:
		case DISCARD_RESULT_ID:
		case BREAKPOINTS_REQ_ID:
			delete r;
			break;
	}
}

void DebuggerForm::handleUpdate(UpdateMessage *m)
{
	if(m->type == "status" && m->name == "paused") {
		pauseStatusChanged(m->result == "true");
	} else if (m->type == "break" && m->name == "pc") {
		breakOccured( m->result.toShort(0,0) );
	}
	delete m;
}

void DebuggerForm::pauseStatusChanged(bool isPaused) 
{
	systemPauseAction->setChecked(isPaused);
}

void DebuggerForm::breakOccured(quint16)
{
	setBreakMode();

	// refresh breakpoints 
	CommCommandRequest *req = new CommCommandRequest(BREAKPOINTS_REQ_ID, "debug list_bp");
	comm.getCommandResult(req);

	// update registers 
	// note that a register update is processed, a signal is sent to other
	// widgets as well. Any dependent updates shoud be called before this one.
	CommDebuggableRequest *regs = new CommDebuggableRequest(CPUREG_REQ_ID, "{CPU regs}", 0, 28, NULL, 0);
	regs->target = (unsigned char *)new Z80Registers;
	comm.getDebuggableData(regs);
	
	// refresh memory viewer
	hexView->refresh();
}

void DebuggerForm::setBreakMode()
{
	executeBreakAction->setEnabled(FALSE);
	executeRunAction->setEnabled(TRUE);
	executeStepAction->setEnabled(TRUE);
	executeStepOverAction->setEnabled(TRUE);
	executeStepOutAction->setEnabled(TRUE);
	executeRunToAction->setEnabled(TRUE);
}

void DebuggerForm::setRunMode()
{
	executeBreakAction->setEnabled(TRUE);
	executeRunAction->setEnabled(FALSE);
	executeStepAction->setEnabled(FALSE);
	executeStepOverAction->setEnabled(FALSE);
	executeStepOutAction->setEnabled(FALSE);
	executeRunToAction->setEnabled(FALSE);
}

void DebuggerForm::systemConnect()
{
	systemConnectAction->setEnabled(FALSE);
	comm.connectToOpenMSX("localhost");
}

void DebuggerForm::systemDisconnect()
{
	comm.closeConnection();
}

void DebuggerForm::systemPause()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "set pause ");
	if( systemPauseAction->isChecked() )
		r->command.append("true");
	else
		r->command.append("false");
	comm.getCommandResult(r);
}

void DebuggerForm::executeBreak()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "debug break");
	comm.getCommandResult(r);
}

void DebuggerForm::executeRun()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "debug cont");
	comm.getCommandResult(r);
	setRunMode();
}

void DebuggerForm::executeStep()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "debug step");
	comm.getCommandResult(r);
	setRunMode();
}

void DebuggerForm::executeStepOver()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "step_over");
	comm.getCommandResult(r);
	setRunMode();
}

void DebuggerForm::executeRunTo()
{
	CommCommandRequest *r = new CommCommandRequest(DISCARD_RESULT_ID, "");
	r->command.setNum(disasmView->cursorAddr, 16);
	r->command.prepend("run_to 0x");
	comm.getCommandResult(r);
	setRunMode();
}

void DebuggerForm::executeStepOut()
{
}

void DebuggerForm::showAbout()
{
	QString s;
	s.sprintf("openMSX debugger %i.%i.%i", VERSION_MAJOR,
	                                       VERSION_MINOR,
	                                       VERSION_PATCH);

	QMessageBox::about(this, "openMSX", s);
}
