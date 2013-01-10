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
		dataStore.old_version = false;
		dataStore.got_version = true;
		dataStore.refresh();
		delete this;
	}
	virtual void replyNok(const QString& message)
	{
		dataStore.old_version = true;
		dataStore.got_version = true;
		dataStore.refresh();
		delete this;
	}

private:
	VDPDataStore& dataStore;
};


static const unsigned TOTAL_SIZE = 0x20000 + 32 + 16 + 64 + 2;

VDPDataStore::VDPDataStore()
{
	vram = new unsigned char[TOTAL_SIZE];
	memset(vram, 0x00, TOTAL_SIZE);
	got_version = false;
	old_version = false;
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

	QString req;
	if (old_version) {
		req = QString(
			"debug_bin2hex "
			"[ debug read_block {VRAM} 0 0x20000 ]"
			"[ debug read_block {VDP palette} 0 32 ]"
			"[ debug read_block {VDP status regs} 0 16 ]"
			"[ debug read_block {VDP regs} 0 64 ]"
			"[ debug read_block {VRAM pointer} 0 2 ]");
	} else {
		req = QString(
			"debug_bin2hex "
			"[ debug read_block {physical VRAM} 0 0x20000 ]"
			"[ debug read_block {VDP palette} 0 32 ]"
			"[ debug read_block {VDP status regs} 0 16 ]"
			"[ debug read_block {VDP regs} 0 64 ]"
			"[ debug read_block {VRAM pointer} 0 2 ]");
	}
	new SimpleHexRequest(req, TOTAL_SIZE, vram, *this);
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
	return vram + 0x20000;
}
const unsigned char* VDPDataStore::getStatusRegsPointer() const
{
	return vram + 0x20000 + 32;
}
const unsigned char* VDPDataStore::getRegsPointer() const
{
	return vram + 0x20000 + 32 + 16;
}
const unsigned char* VDPDataStore::getVdpVramPointer() const
{
	return vram + 0x20000 + 32 + 16 + 64;
}
