#include "VDPDataStore.h"
#include "CommClient.h"

class VDPDataStoreVersionCheck : public SimpleCommand
{
public:
	VDPDataStoreVersionCheck(VDPDataStore& dataStore_)
		: SimpleCommand("debug desc {physical VRAM}")
		, dataStore(dataStore_)
	{
	}

	virtual void replyOk(const QString& message)
	{
		dataStore.debuggableNameVRAM = "physical VRAM";
		dataStore.got_version = true;
		dataStore.refresh();
		delete this;
	}
	virtual void replyNok(const QString& message)
	{
		dataStore.debuggableNameVRAM = "VRAM";
		dataStore.got_version = true;
		dataStore.refresh();
		delete this;
	}

private:
	VDPDataStore& dataStore;
};

class VDPDataStoreVRAMSizeCheck : public SimpleCommand
{
public:
	VDPDataStoreVRAMSizeCheck(VDPDataStore& dataStore_)
		: SimpleCommand("debug size {" + QString::fromStdString(dataStore_.debuggableNameVRAM) + "}")
		, dataStore(dataStore_)
	{
	}

	virtual void replyOk(const QString& message)
	{
		dataStore.vramSize = message.toInt();
		dataStore.refresh2();
		delete this;
	}
	virtual void replyNok(const QString& message)
	{
		delete this;
	}

private:
	VDPDataStore& dataStore;
};


static const unsigned MAX_VRAM_SIZE = 0x30000;
static const unsigned MAX_TOTAL_SIZE = MAX_VRAM_SIZE + 32 + 16 + 64 + 2;

VDPDataStore::VDPDataStore()
{
	vram = new unsigned char[MAX_TOTAL_SIZE];
	memset(vram, 0x00, MAX_TOTAL_SIZE);
	got_version = false;
	CommClient::instance().sendCommand(new VDPDataStoreVersionCheck(*this));
}

VDPDataStore::~VDPDataStore()
{
	delete[] vram;
}

VDPDataStore& VDPDataStore::instance()
{
	static VDPDataStore oneInstance;
	return oneInstance;
}

void VDPDataStore::refresh()
{
	if (!got_version) return;

	refresh1();
}

void VDPDataStore::refresh1()
{
	CommClient::instance().sendCommand(new VDPDataStoreVRAMSizeCheck(*this));
}

void VDPDataStore::refresh2()
{
	QString req = QString(
			"debug_bin2hex "
			"[ debug read_block {" + QString::fromStdString(debuggableNameVRAM) + "} 0 " + QString::number(vramSize) + " ]"
			"[ debug read_block {VDP palette} 0 32 ]"
			"[ debug read_block {VDP status regs} 0 16 ]"
			"[ debug read_block {VDP regs} 0 64 ]"
			"[ debug read_block {VRAM pointer} 0 2 ]");
	new SimpleHexRequest(req, MAX_TOTAL_SIZE - MAX_VRAM_SIZE + vramSize, vram, *this);
}

void VDPDataStore::DataHexRequestReceived()
{
	emit dataRefreshed();
}


const unsigned char* VDPDataStore::getVramPointer() const
{
	return vram;
}
const unsigned char* VDPDataStore::getPalettePointer() const
{
	return vram + vramSize;
}
const unsigned char* VDPDataStore::getStatusRegsPointer() const
{
	return vram + vramSize + 32;
}
const unsigned char* VDPDataStore::getRegsPointer() const
{
	return vram + vramSize + 32 + 16;
}
const unsigned char* VDPDataStore::getVdpVramPointer() const
{
	return vram + vramSize + 32 + 16 + 64;
}

const size_t VDPDataStore::getVRAMSize() const
{
	return vramSize;
}
