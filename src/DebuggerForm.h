// $Id$

#ifndef DEBUGGERFORM_H
#define DEBUGGERFORM_H

#include "DockManager.h"
#include "DebuggerData.h"
#include "SymbolTable.h"
#include <QMainWindow>

class DockableWidgetArea;
class DisasmViewer;
class HexViewer;
class CPURegsViewer;
class FlagsViewer;
class StackViewer;
class SlotViewer;
class CommClient;
class QAction;
class QMenu;
class QToolBar;
class QSplitter;

class DebuggerForm : public QMainWindow
{
	Q_OBJECT;
public:
	DebuggerForm(QWidget* parent = 0);
	~DebuggerForm();

public slots:
	void showAbout();

private:
	QMenu* systemMenu;
	QMenu* viewMenu;
	QMenu* executeMenu;
	QMenu* breakpointMenu;
	QMenu* helpMenu;

	QToolBar* systemToolbar;
	QToolBar* executeToolbar;

	QAction* systemConnectAction;
	QAction* systemDisconnectAction;
	QAction* systemPauseAction;
	QAction* systemSymbolManagerAction;
	QAction* systemPreferencesAction;
	QAction* systemQuitAction;

	QAction* viewRegistersAction;
	QAction* viewFlagsAction;
	QAction* viewStackAction;
	QAction* viewSlotsAction;
	QAction* viewMemoryAction;

	QAction* executeBreakAction;
	QAction* executeRunAction;
	QAction* executeStepAction;
	QAction* executeStepOverAction;
	QAction* executeRunToAction;
	QAction* executeStepOutAction;

	QAction* breakpointToggleAction;
	QAction* breakpointAddAction;

	QAction* helpAboutAction;

	DockManager dockMan;
	DockableWidgetArea *mainArea;
	
	DisasmViewer* disasmView;
	HexViewer* hexView;
	CPURegsViewer* regsView;
	FlagsViewer* flagsView;
	StackViewer* stackView;
	SlotViewer* slotView;

	CommClient& comm;
	Breakpoints breakpoints;
	MemoryLayout memLayout;
	SymbolTable symTable;
	unsigned char* mainMemory;

	QList<DockableWidget*> dockWidgets;

	void createActions();
	void createMenus();
	void createToolbars();
	void createStatusbar();
	void createForm();

	void finalizeConnection(bool halted);
	void pauseStatusChanged(bool isPaused);
	void breakOccured();
	void setBreakMode();
	void setRunMode();
	void updateData();
	
	void refreshBreakpoints();
	DockableWidget *findDockableWidget( const QString& id );

private slots:
	void systemConnect();
	void systemDisconnect();
	void systemPause();
	void systemSymbolManager();
	void systemPreferences();
	void toggleRegisterDisplay();
	void toggleFlagsDisplay();
	void toggleStackDisplay();
	void toggleSlotsDisplay();
	void toggleMemoryDisplay();
	void executeBreak();
	void executeRun();
	void executeStep();
	void executeStepOver();
	void executeRunTo();
	void executeStepOut();
	void breakpointToggle(int addr = -1);
	void breakpointAdd();

	void toggleView( DockableWidget* widget );
	void initConnection();
	void handleUpdate(const QString& type, const QString& name, const QString& message);
	void connectionClosed();
	void dockWidgetVisibilityChanged( DockableWidget *w );
	void updateViewMenu();

	friend class QueryPauseHandler;
	friend class QueryBreakedHandler;
	friend class ListBreakPointsHandler;
	friend class CPURegRequest;
};

#endif // DEBUGGERFORM_H
