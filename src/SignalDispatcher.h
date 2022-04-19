#pragma once

#include "CPURegs.h"
#include "DebuggerData.h"

#include <QLabel>


/** \brief A singleton signal dispatcher
 *
 * Several debugger widgets like 
 * This signal dispatcher also does the CPU registers tracking since multi widgets need those even if there wouldn't be a CPURegsViewer
 */
class SignalDispatcher : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SignalDispatcher)
public:
    /** \brief Dispatcher getter
     *
     * This is a singleton class, i. e. you can't construct any object yourself. To get the one-and-only instance of this class, you need to call SignalDispatcher::getDispatcher(). The function will create the object if neccessary (= when called for the first time) and return a pointer to it.
     * \return A pointer to the one-and-only instance of SignalDispatcher
     */
    static SignalDispatcher* getDispatcher();
    ~SignalDispatcher();

    unsigned char* getMainMemory();
    MemoryLayout*  getMemLayout();

    int readRegister(int id);
    void setData(unsigned char* datPtr);

public slots:
    void refresh();

private:
    static SignalDispatcher* theDispatcher;
    SignalDispatcher();

    //buffers to handle tracking of the CPU registers
    int regs[16], regsCopy[16];
    bool regsModified[16];
    bool regsChanged[16];
    void setRegister(int id, int value);
    void getRegister(int id, unsigned char* data);

    //main 64K used by disasmview and stack
    unsigned char* mainMemory;
    MemoryLayout memLayout;

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
    void updateSlots(const QString& message);
    //signals from/for the diasmView
    void toggleBreakpoint(uint16_t adr);
    void breakpointToggled(uint16_t adr);
    void setProgramCounter(uint16_t pc, bool reload = false);
    //from the GotoDialog
    void setCursorAddress(uint16_t addr, int infoLine, int method);
};
