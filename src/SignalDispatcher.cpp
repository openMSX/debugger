#include "SignalDispatcher.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "VDPDataStore.h"

class DispatchDebugMemMapperHandler : public SimpleCommand
{
public:
    DispatchDebugMemMapperHandler(SignalDispatcher& viewer_)
        : SimpleCommand("debug_memmapper"),
          viewer(viewer_)
    {
    }

    void replyOk(const QString& message) override
    {
        emit viewer.updateSlots(message);
        delete this;
    }
private:
    SignalDispatcher& viewer;
};

SignalDispatcher* SignalDispatcher::theDispatcher=nullptr;

SignalDispatcher* SignalDispatcher::getDispatcher()
{
    if (theDispatcher==nullptr) {
			theDispatcher = new SignalDispatcher{};
	};
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
    if (isEnableWidget != value){
        isEnableWidget=value;
        emit enableWidget(value);
    }
}

int SignalDispatcher::readRegister(int id)
{
    return regs[id];
}

SignalDispatcher::SignalDispatcher(): isEnableWidget(false) {
    // avoid UMR
    memset(&regs,         0, sizeof(regs));
    memset(&regsChanged,  0, sizeof(regsChanged));
    memset(&regsModified, 0, sizeof(regsModified));

    // init main memory
    // added four bytes as runover buffer for dasm
    // otherwise dasm would need to check the buffer end continously.
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

void SignalDispatcher::getRegister(int id, unsigned char *data)
{
    data[0] = regs[id] >> 8;
    data[1] = regs[id] & 255;
}

