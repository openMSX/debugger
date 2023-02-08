#ifndef DEBUGGERFORM_H
#define DEBUGGERFORM_H

#include "DockableWidgetArea.h"
#include "DockManager.h"
#include "DebugSession.h"
#include "SymbolManager.h"
#include "CommandDialog.h"
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <cstdint>
#include <memory>

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
		uint8_t ps;
		std::optional<uint8_t> ss;
		std::optional<uint8_t> segment;
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
	QMenu* commandMenu;
	QMenu* helpMenu;
	QDialog* commandDialog;

	QToolBar* systemToolbar;
	QToolBar* executeToolbar;
	QToolBar* userToolbar;

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
	QAction* commandAction;
	QAction* helpAboutAction;

	DockManager dockMan;
	std::unique_ptr<DockableWidgetArea> mainArea;
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
	QPointer<SymbolManager> symManager;

	CommClient& comm;
	DebugSession session;
	MemoryLayout memLayout;
	uint8_t mainMemory[0x10000 + 4] = {}; // 4 extra to avoid wrap-check during disasm

	bool mergeBreakpoints;
	QMap<QString, int> debuggables;

	static int counter;
	enum {RESET = 0, SLOTS_CHECKED, PC_CHANGED, SLOTS_CHANGED} disasmStatus = RESET;
	uint16_t disasmAddress;

	QList<CommandRef> commands;
	void updateCustomActions();

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

	void toggleBreakpoint();
	void toggleBreakpointAddress(uint16_t addr);
	void addBreakpoint();

	void manageCommandButtons();
	void manageCommandButtonsFinished(int result);

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

	QByteArray saveCommands() const;
	void restoreCommands(const QByteArray& input);

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
	void symbolFilesChanged();
	void runStateEntered();
	void breakStateEntered();
	void breakpointsUpdated();
	void debuggablesChanged(const QMap<QString, int>& list);
};

#endif // DEBUGGERFORM_H
