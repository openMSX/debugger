#ifndef SIGNALDISPATCHER_H
#define SIGNALDISPATCHER_H

#include "CPURegs.h"
#include "DebuggerData.h"
#include <QLabel>
#include <cstdint>


/** \brief A singleton signal dispatcher
 *
 * Several debugger widgets like
 * This signal dispatcher also does the CPU registers tracking since multi widgets need those even if there wouldn't be a CPURegsViewer
 */
class SignalDispatcher : public QObject
{
    Q_OBJECT

public:
    SignalDispatcher(const SignalDispatcher&) = delete;
    SignalDispatcher& operator=(const SignalDispatcher&) = delete;

    static SignalDispatcher& instance();

    uint8_t* getMainMemory();
    MemoryLayout* getMemLayout();

    int readRegister(int id);
    void setData(uint8_t* datPtr);

    bool getEnableWidget();

public slots:
    void refresh();
    void setEnableWidget(bool value);
    void updateSlots(const QString& message);

private:
    SignalDispatcher() = default;
    ~SignalDispatcher() = default;

    void setRegister(int id, int value);
    void getRegister(int id, uint8_t* data);

signals:
    void enableWidget(bool enable);
	void connected();
	void settingsChanged();
	void symbolsChanged();
	void runStateEntered();
	void breakStateEntered();
	void breakpointsUpdated();
	void debuggablesChanged(const QMap<QString, int>& list);
    // signals concerning CPU registers
    void registersUpdate(unsigned char* datPtr);
    void registerChanged(int id, int value);
    void pcChanged(uint16_t);
    void flagsChanged(quint8);
    void spChanged(quint16);
    // signals concerning slotselection
    void slotsUpdated(bool slotsChanged);
    //signals from/for the disasmView
    void toggleBreakpoint(uint16_t adr);
    void breakpointToggled(uint16_t adr);
    void setProgramCounter(uint16_t pc, bool reload = false);
    //from the GotoDialog
    void setCursorAddress(uint16_t addr, int infoLine, int method);

private:
    //main 64K used by disasmview and stack
    uint8_t mainMemory[65536 + 4] = {}; // 4 extra to avoid bound-checks during disassembly
    MemoryLayout memLayout;

    //buffers to handle tracking of the CPU registers
    int regs[16] = {};
    int regsCopy[16];
    bool regsModified[16] = {};
    bool regsChanged[16] = {};

    //buffers to handle tracking of the slots layout
    enum {RESET = 0, SLOTS_CHECKED, PC_CHANGED, SLOTS_CHANGED} disasmStatus = RESET;
    bool slotsChanged[4];
    bool segmentsChanged[4];

    bool isEnableWidget = false;
};

#endif
