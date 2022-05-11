#ifndef DEBUGGERFORM_H
#define DEBUGGERFORM_H

#include "DebugSession.h"
#include <QMainWindow>
#include <QMap>

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
class BlendSplitter;
class QTabWidget;
class TabRenamerHelper;

class DebuggerForm : public QMainWindow
{
	Q_OBJECT
public:
	DebuggerForm(QWidget* parent = nullptr);
	~DebuggerForm() override;

	void showAbout();
	void reloadBreakpoints(bool merge = false);
	void onSlotsUpdated(bool slotsChanged);

    enum factoryclasses {
        disasmViewer,
        mainMemoryViewer,
        cpuRegsViewer,
        flagsViewer,
        stackViewer,
        slotViewer,
        breakpointViewer,
        debuggableViewer,
        vdpStatusRegViewer,
        vdpCommandRegViewer,
        bitMapViewer,
        tileViewer,
        spriteViewer,
        vdpRegisters,
        quickguide,
        paletteViewer
    };

    bool saveWorkspacesAs(const QString& newFileName);
    bool loadWorkspaces(const QString& filename);
    QString fileSaveWorkspaceAs();

private:
	void closeEvent(QCloseEvent* e) override;

	void createWidgetRegistry();
	void createActions();
	void createMenus();
	void createToolbars();
	void createStatusbar();
	void createForm();

    static QWidget* widgetFactory(factoryclasses fctwidget);

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

	QMenu* executeMenu;
	QMenu* breakpointMenu;
	QMenu* helpMenu;

	QToolBar* systemToolbar;
	QToolBar* executeToolbar;

	QAction* fileNewSessionAction;
	QAction* fileOpenSessionAction;
	QAction* fileSaveSessionAction;
	QAction* fileSaveSessionAsAction;

    QAction* fileOpenWorkspaceLayoutAction;
    QAction* fileSaveWorkspaceLayoutAction;
    QAction* fileSaveWorkspaceLayoutAsAction;

	QAction* fileQuitAction;

    static const int MaxRecentFiles = 5;
	QAction* recentFileActions[MaxRecentFiles];
	QAction* recentFileSeparator;

	QAction* systemConnectAction;
	QAction* systemDisconnectAction;
	QAction* systemPauseAction;
	QAction* systemRebootAction;
	QAction* systemSymbolManagerAction;
	QAction* systemPreferencesAction;

	QAction* searchGotoAction;
	QAction* executeBreakAction;
	QAction* executeRunAction;
	QAction* executeStepAction;
	QAction* executeStepOverAction;
	QAction* executeStepOutAction;
	QAction* executeStepBackAction;

	QAction* breakpointAddAction;

	QAction* helpAboutAction;

    QAction* addCPUWorkspaceAction;
    QAction* addVDPRegsWorkspaceAction;
    QAction* addVDPTilesWorkspaceAction;
    QAction* addVDPBitmapWorkspaceAction;
    QAction* addEmptyWorkspaceAction;
    QAction* addFloatingSwitchingWidgetAction;

	QStringList recentFiles;

	CommClient& comm;
    DebugSession* session;

    QTabWidget *workspaces;
    TabRenamerHelper *tabRenamer;

	bool mergeBreakpoints;
    static QMap<QString, int> debuggables;

	static int counter;
    uint16_t disasmAddress;


	void fileNewSession();
	void fileOpenSession();
	void fileSaveSession();
	void fileSaveSessionAs();
	void fileRecentOpen();
    void fileOpenWorkspace();
    void fileSaveWorkspace();

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
//	void executeRunTo();
	void executeStepOut();
	void executeStepBack();
private slots:
    void toggleBreakpoint(uint16_t addr);
private:
    void addBreakpoint(uint16_t cursorAddress);
	void initConnection();
	void handleUpdate(const QString& type, const QString& name,
	                  const QString& message);
	void setDebuggables(const QString& list);
	void setDebuggableSize(const QString& debuggable, int size);
	void connectionClosed();
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

    friend class TabRenamerHelper;

signals:
	void connected();
	void settingsChanged();
	void symbolsChanged();
	void runStateEntered();
	void breakStateEntered();
	void breakpointsUpdated();
    void debuggablesChanged(const QMap<QString, int>& list);

protected:
    BlendSplitter *createWorkspaceCPU();
    BlendSplitter *createWorkspaceVDPRegs();
    BlendSplitter *createWorkspaceVDPTiles();
    BlendSplitter *createWorkspaceVDPBitmap();
    void addInfoWorkspace();
    void addCPUWorkspace();
    void addVDPRegsWorkspace();
    void addVDPTilesWorkspace();
    void addVDPBitmapWorkspace();
    void addEmptyWorkspace();
    void addFloatingSwitchingWidget();
    void addDefaultWorkspaces();

protected slots:
    void tabCloseRequest(int index);

};

#endif // DEBUGGERFORM_H
