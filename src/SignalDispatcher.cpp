#include "SignalDispatcher.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "VDPDataStore.h"

class DispatchDebugMemMapperHandler : public SimpleCommand
{
public:
    DispatchDebugMemMapperHandler(SignalDispatcher& dispatcher_)
        : SimpleCommand("debug_memmapper"),
          dispatcher(dispatcher_)
    {
    }

    void replyOk(const QString& message) override
    {
        dispatcher.updateSlots(message);
        delete this;
    }

private:
    SignalDispatcher& dispatcher;
};

SignalDispatcher* SignalDispatcher::theDispatcher = nullptr;

SignalDispatcher* SignalDispatcher::getDispatcher()
{
    if (theDispatcher == nullptr) {
			theDispatcher = new SignalDispatcher{};
	}
    return theDispatcher;
}

SignalDispatcher::~SignalDispatcher()
{
    delete[] mainMemory;
}

unsigned char *SignalDispatcher::getMainMemory()
{
    return mainMemory;
}

MemoryLayout* SignalDispatcher::getMemLayout()
{
    return &memLayout;
}

void SignalDispatcher::setData(unsigned char *datPtr)
{
    setRegister(CpuRegs::REG_AF , datPtr[ 0] * 256 + datPtr[ 1]);
    setRegister(CpuRegs::REG_BC , datPtr[ 2] * 256 + datPtr[ 3]);
    setRegister(CpuRegs::REG_DE , datPtr[ 4] * 256 + datPtr[ 5]);
    setRegister(CpuRegs::REG_HL , datPtr[ 6] * 256 + datPtr[ 7]);
    setRegister(CpuRegs::REG_AF2, datPtr[ 8] * 256 + datPtr[ 9]);
    setRegister(CpuRegs::REG_BC2, datPtr[10] * 256 + datPtr[11]);
    setRegister(CpuRegs::REG_DE2, datPtr[12] * 256 + datPtr[13]);
    setRegister(CpuRegs::REG_HL2, datPtr[14] * 256 + datPtr[15]);
    setRegister(CpuRegs::REG_IX , datPtr[16] * 256 + datPtr[17]);
    setRegister(CpuRegs::REG_IY , datPtr[18] * 256 + datPtr[19]);
    setRegister(CpuRegs::REG_PC , datPtr[20] * 256 + datPtr[21]);
    setRegister(CpuRegs::REG_SP , datPtr[22] * 256 + datPtr[23]);
    setRegister(CpuRegs::REG_I  , datPtr[24]);
    setRegister(CpuRegs::REG_R  , datPtr[25]);
    setRegister(CpuRegs::REG_IM , datPtr[26]);

    // IFF separately to only check bit 0 for change
    regsChanged[CpuRegs::REG_IFF] = (regs[CpuRegs::REG_IFF] & 1) != (datPtr[27] & 1);
    regs[CpuRegs::REG_IFF] = datPtr[27];

    // reset modifications
    memset(&regsModified, 0, sizeof(regsModified));
    memcpy(&regsCopy, &regs, sizeof(regs));

    emit pcChanged(regs[CpuRegs::REG_PC]);
    emit spChanged(regs[CpuRegs::REG_SP]);
    emit flagsChanged(regs[CpuRegs::REG_AF] & 0xFF);
    emit registersUpdate(datPtr); // now tell all listeners the new values

    //now signals for the disasmView
    if (disasmStatus != RESET) {
        //disasmView->setProgramCounter(address, disasmStatus == SLOTS_CHANGED);
    } else {
        disasmStatus = PC_CHANGED;
    }
    emit setProgramCounter(regs[CpuRegs::REG_PC], disasmStatus == SLOTS_CHANGED);
}

void SignalDispatcher::updateSlots(const QString& message)
{
    QStringList lines = message.split('\n');
    bool changed = false;

    // parse page slots and segments
    for (int p = 0; p < 4; ++p) {
        bool subSlotted = (lines[p * 2][1] != 'X');
        slotsChanged[p] = (memLayout.primarySlot  [p] != lines[p * 2][0].toLatin1()-'0') ||
                          (memLayout.secondarySlot[p] != (subSlotted ? lines[p * 2][1].toLatin1() - '0' : -1));
        changed |= slotsChanged[p];
        memLayout.primarySlot  [p] = lines[p * 2][0].toLatin1()-'0';
        memLayout.secondarySlot[p] = subSlotted
            ? lines[p * 2][1].toLatin1() - '0' : -1;
        segmentsChanged[p] = memLayout.mapperSegment[p] !=
                             lines[p * 2 + 1].toInt();
        memLayout.mapperSegment[p] = lines[p * 2 + 1].toInt();
    }
    // parse slot layout
    int l = 8;
    for (int ps = 0; ps < 4; ++ps) {
        memLayout.isSubslotted[ps] = lines[l++][0] == '1';
        if (memLayout.isSubslotted[ps]) {
            for (int ss = 0; ss < 4; ++ss) {
                memLayout.mapperSize[ps][ss] = lines[l++].toUShort();
            }
        } else {
            memLayout.mapperSize[ps][0] = lines[l++].toUShort();
        }
    }
    // parse rom blocks
    for (int i = 0; i < 8; ++i, ++l) {
        if (lines[l][0] == 'X') {
            memLayout.romBlock[i] = -1;
        } else {
            memLayout.romBlock[i] = lines[l].toInt();
        }
    }

    //signals to update disasmview
    if (disasmStatus == PC_CHANGED) {
        //disasmView->setProgramCounter(disasmAddress, slotsChanged);
        emit setProgramCounter(regs[CpuRegs::REG_PC], changed);
        disasmStatus = RESET;
    } else {
        disasmStatus = changed ? SLOTS_CHANGED : SLOTS_CHECKED;
    }

    emit slotsUpdated(changed);
}

bool SignalDispatcher::getEnableWidget()
{
    return isEnableWidget;
}

void SignalDispatcher::refresh()
{
    CommClient::instance().sendCommand(new DispatchDebugMemMapperHandler(*this));
    VDPDataStore::instance().refresh();
}

void SignalDispatcher::setEnableWidget(bool value)
{
    if (isEnableWidget != value) {
        isEnableWidget = value;
        emit enableWidget(value);
    }
}

int SignalDispatcher::readRegister(int id)
{
    return regs[id];
}

SignalDispatcher::SignalDispatcher()
{
    // avoid UMR
    memset(&regs,         0, sizeof(regs));
    memset(&regsChanged,  0, sizeof(regsChanged));
    memset(&regsModified, 0, sizeof(regsModified));

    // init main memory
    // added four bytes as runover buffer for dasm
    // otherwise dasm would need to check the buffer end continuously.
    mainMemory = new unsigned char[65536 + 4];
    memset(mainMemory, 0, 65536 + 4);
}

void SignalDispatcher::setRegister(int id, int value)
{
    regsChanged[id] = regs[id] != value;
    regs[id] = value;
    if (regsChanged[id]) {
        emit registerChanged(id, value);
    }
}

void SignalDispatcher::getRegister(int id, unsigned char* data)
{
    data[0] = regs[id] >> 8;
    data[1] = regs[id] & 255;
}
