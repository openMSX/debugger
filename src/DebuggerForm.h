// $Id$

#ifndef DEBUGGERFORM_H
#define DEBUGGERFORM_H

#include "DockManager.h"
#include "DebugSession.h"
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

protected:
	void closeEvent( QCloseEvent *e );

public slots:
	void showAbout();

private:
	QMenu* fileMenu;
	QMenu* systemMenu;
	QMenu* viewMenu;
	QMenu* executeMenu;
	QMenu* breakpointMenu;
	QMenu* helpMenu;

	QToolBar* systemToolbar;
	QToolBar* executeToolbar;

	QAction* fileNewSessionAction;
	QAction* fileOpenSessionAction;
	QAction* fileSaveSessionAction;
	QAction* fileSaveSessionAsAction;
	QAction* fileQuitAction;

	QAction* systemConnectAction;
	QAction* systemDisconnectAction;
	QAction* systemPauseAction;
	QAction* systemRebootAction;
	QAction* systemSymbolManagerAction;
	QAction* systemPreferencesAction;

	QAction* viewRegistersAction;
	QAction* viewFlagsAction;
	QAction* viewStackAction;
	QAction* viewSlotsAction;
	QAction* viewMemoryAction;
	QAction* viewDebuggableViewerAction;
	
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
	DebugSession session;
	MemoryLayout memLayout;
	unsigned char* mainMemory;

	QMap<QString, int> debuggables;
	
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

private slots:
	void fileNewSession();
	void fileOpenSession();
	void fileSaveSession();
	void fileSaveSessionAs();
	void systemConnect();
	void systemDisconnect();
	void systemPause();
	void systemReboot();
	void systemSymbolManager();
	void systemPreferences();
	void toggleRegisterDisplay();
	void toggleFlagsDisplay();
	void toggleStackDisplay();
	void toggleSlotsDisplay();
	void toggleMemoryDisplay();
	void addDebuggableViewer();
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
	void setDebuggables( const QString& list );
	void setDebuggableSize(  const QString& debuggable, int size );
	void connectionClosed();
	void dockWidgetVisibilityChanged( DockableWidget *w );
	void updateViewMenu();

	friend class QueryPauseHandler;
	friend class QueryBreakedHandler;
	friend class ListBreakPointsHandler;
	friend class CPURegRequest;
	friend class ListDebuggablesHandler;
	friend class DebuggableSizeHandler;
	
signals:
	void settingsChanged();
	void symbolsChanged();
	void debuggablesChanged( const QMap<QString,int>& list );
	void emulationChanged();
};

#endif // DEBUGGERFORM_H
