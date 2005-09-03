// $Id$

#ifndef _DEBUGGERFORM_H
#define _DEBUGGERFORM_H

#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QMap>

#include "DisasmViewer.h"
#include "HexViewer.h"
#include "CPURegsViewer.h"
#include "FlagsViewer.h"
#include "CommClient.h"
#include "StackViewer.h"

#include "DebuggerData.h"


class DebuggerForm : public QMainWindow
{
	Q_OBJECT;
public:
	DebuggerForm( QWidget* parent = 0 );
    ~DebuggerForm();

public slots:
	void showAbout();

private:
	QMenu *systemMenu;
	QMenu *executeMenu;
	QMenu *helpMenu;

	QToolBar *systemToolbar;
	QToolBar *executeToolbar;

	QAction *systemConnectAction;
	QAction *systemDisconnectAction;
	QAction *systemPauseAction;
	QAction *systemExitAction;

	QAction *executeBreakAction;
	QAction *executeRunAction;
	QAction *executeStepAction;
	QAction *executeStepOverAction;
	QAction *executeRunToAction;
	QAction *executeStepOutAction;

	QAction *helpAboutAction;

	QSplitter *mainSplitter;
	QSplitter *disasmSplitter;

 	DisasmViewer *disasmView;
	HexViewer *hexView;
	CPURegsViewer *regsView;
	FlagsViewer *flagsView;
	StackViewer *stackView;

	CommClient comm;
	Breakpoints breakpoints;
	unsigned char *mainMemory;
	
	void createActions();
	void createMenus();
	void createToolbars();
	void createStatusbar();
	void createForm();
	QWidget *createNamedWidget(const QString& name, QWidget *widget);

	void finalizeConnection(bool halted);
	void pauseStatusChanged(bool isPaused);
	void breakOccured(quint16);
	void setBreakMode();
	void setRunMode();

private slots:
	void systemConnect();
	void systemDisconnect();
	void systemPause();
	void executeBreak();
	void executeRun();
	void executeStep();
	void executeStepOver();
	void executeRunTo();
	void executeStepOut();

	void initConnection();
	void dataTransfered(CommRequest *r);
	void cancelTransfer(CommRequest *r);
	void handleUpdate(UpdateMessage *m);
	void connectionClosed();
	void handleError( CommClient::ConnectionError error );

};

#endif    // _DEBUGGERFORM_H
