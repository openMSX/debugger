#ifndef DEBUGGERFORM_H
#define DEBUGGERFORM_H

#include "DockManager.h"
#include "DebugSession.h"
#include <QMainWindow>
#include <QMap>

class DockableWidgetArea;
class DisasmViewer;
class MainMemoryViewer;
class CPURegsViewer;
class FlagsViewer;
class StackViewer;
class SlotViewer;
class CommClient;
class QAction;
class QMenu;
class QToolBar;
class VDPStatusRegViewer;
class VDPRegViewer;
class VDPCommandRegViewer;
class BreakpointViewer;

class DebuggerForm : public QMainWindow
{
	Q_OBJECT
public:
	DebuggerForm(QWidget* parent = nullptr);
	~DebuggerForm() override;

	void showAbout();
	void reloadBreakpoints(bool merge = false);
	void onSlotsUpdated(bool slotsChanged);
	void onPCChanged(uint16_t address);

private:
	void closeEvent(QCloseEvent* e) override;

	void createActions();
	void createMenus();
	void createToolbars();
	void createStatusbar();
	void createForm();

	void openSession(const QString& file);
	void updateRecentFiles();
	void addRecentFile(const QString& file);
	void removeRecentFile(const QString& file);

	void finalizeConnection(bool halted);
	void pauseStatusChanged(bool isPaused);
	void breakOccured();
	void setRunMode();
	void updateData();

	void refreshBreakpoints();

	struct AddressSlotResult {
		qint8 ps;
		qint8 ss;
		int segment;
	};
	AddressSlotResult addressSlot(int addr) const;

	QMenu* fileMenu;
	QMenu* systemMenu;
	QMenu* searchMenu;
	QMenu* viewMenu;
	QMenu* viewVDPDialogsMenu;
	QMenu* viewFloatingWidgetsMenu;
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

	enum { MaxRecentFiles = 5 };
	QAction* recentFileActions[MaxRecentFiles];
	QAction* recentFileSeparator;

	QAction* systemConnectAction;
	QAction* systemDisconnectAction;
	QAction* systemPauseAction;
	QAction* systemRebootAction;
	QAction* systemSymbolManagerAction;
	QAction* systemPreferencesAction;

	QAction* searchGotoAction;

	QAction* viewRegistersAction;
	QAction* viewFlagsAction;
	QAction* viewStackAction;
	QAction* viewSlotsAction;
	QAction* viewMemoryAction;
	QAction* viewBreakpointsAction;
	QAction* viewDebuggableViewerAction;

	QAction* viewBitMappedAction;
	QAction* viewCharMappedAction;
	QAction* viewSpritesAction;
	QAction* viewVDPStatusRegsAction;
	QAction* viewVDPRegsAction;
	QAction* viewVDPCommandRegsAction;

	QAction* executeBreakAction;
	QAction* executeRunAction;
	QAction* executeStepAction;
	QAction* executeStepOverAction;
	QAction* executeRunToAction;
	QAction* executeStepOutAction;
	QAction* executeStepBackAction;

	QAction* breakpointToggleAction;
	QAction* breakpointAddAction;

	QAction* helpAboutAction;

	DockManager dockMan;
	DockableWidgetArea* mainArea;
	QStringList recentFiles;

	DisasmViewer* disasmView;
	MainMemoryViewer* mainMemoryView;
	CPURegsViewer* regsView;
	FlagsViewer* flagsView;
	StackViewer* stackView;
	SlotViewer* slotView;
	VDPStatusRegViewer* VDPStatusRegView;
	VDPRegViewer* VDPRegView;
	VDPCommandRegViewer* VDPCommandRegView;
	BreakpointViewer* bpView;

	CommClient& comm;
	DebugSession session;
	MemoryLayout memLayout;
	unsigned char* mainMemory;

	bool mergeBreakpoints;
	QMap<QString, int> debuggables;

	static int counter;
	enum {RESET = 0, SLOTS_CHECKED, PC_CHANGED, SLOTS_CHANGED} disasmStatus = RESET;
	uint16_t disasmAddress;

	void fileNewSession();
	void fileOpenSession();
	void fileSaveSession();
	void fileSaveSessionAs();
	void fileRecentOpen();
	void systemConnect();
	void systemDisconnect();
	void systemPause();
	void systemReboot();
	void systemSymbolManager();
	void systemPreferences();
	void searchGoto();
	void toggleBreakpointsDisplay();
	void toggleRegisterDisplay();
	void toggleFlagsDisplay();
	void toggleStackDisplay();
	void toggleSlotsDisplay();
	void toggleMemoryDisplay();
	void toggleBitMappedDisplay();
	void toggleCharMappedDisplay();
	void toggleSpritesDisplay();
	void toggleVDPRegsDisplay();
	void toggleVDPStatusRegsDisplay();
	void toggleVDPCommandRegsDisplay();
	void addDebuggableViewer();
	void executeBreak();
	void executeRun();
	void executeStep();
	void executeStepOver();
	void executeRunTo();
	void executeStepOut();
	void executeStepBack();
	void toggleBreakpoint(int addr = -1);
	void addBreakpoint();

	void toggleView(DockableWidget* widget);
	void initConnection();
	void handleUpdate(const QString& type, const QString& name,
	                  const QString& message);
	void setDebuggables(const QString& list);
	void setDebuggableSize(const QString& debuggable, int size);
	void connectionClosed();
	void dockWidgetVisibilityChanged(DockableWidget* w);
	void updateViewMenu();
	void updateVDPViewMenu();
	void updateViewFloatingWidgetsMenu();
	void updateWindowTitle();
	void symbolFileChanged();
	void showFloatingWidget();
	void processBreakpoints(const QString& message);
	void processMerge(const QString& message);

	friend class QueryPauseHandler;
	friend class QueryBreakedHandler;
	friend class ListBreakPointsHandler;
	friend class CPURegRequest;
	friend class ListDebuggablesHandler;
	friend class DebuggableSizeHandler;

signals:
	void connected();
	void settingsChanged();
	void symbolsChanged();
	void runStateEntered();
	void breakStateEntered();
	void breakpointsUpdated();
	void debuggablesChanged(const QMap<QString, int>& list);
};

#endif // DEBUGGERFORM_H
