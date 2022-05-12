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

	void replyOk(const QString& /*message*/) override
	{
		dataStore.debuggableNameVRAM = "physical VRAM";
		dataStore.refresh();
		delete this;
	}
	void replyNok(const QString& /*message*/) override
	{
		dataStore.debuggableNameVRAM = "VRAM";
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
		: SimpleCommand("debug size {" + QString::fromStdString(*dataStore_.debuggableNameVRAM) + "}")
		, dataStore(dataStore_)
	{
	}

	void replyOk(const QString& message) override
	{
		//printf("dataStore.vramSize %i\n",dataStore.vramSize);
		dataStore.vramSize = message.toInt();
		//printf("dataStore.vramSize %i\n",dataStore.vramSize);
        dataStore.refresh2();
		delete this;
	}
	void replyNok(const QString& /*message*/) override
	{
		delete this;
	}

private:
	VDPDataStore& dataStore;
};


static constexpr unsigned MAX_VRAM_SIZE = 0x30000;
static constexpr unsigned MAX_TOTAL_SIZE = MAX_VRAM_SIZE + 32 + 16 + 64 + 2;

VDPDataStore::VDPDataStore()
	: vram(MAX_TOTAL_SIZE)
{
	CommClient::instance().sendCommand(new VDPDataStoreVersionCheck(*this));
}

VDPDataStore& VDPDataStore::instance()
{
	static VDPDataStore oneInstance;
	return oneInstance;
}

void VDPDataStore::refresh()
{
	if (!debuggableNameVRAM) {
		// This can happen when the data store was used before
		// connecting to openMSX
		CommClient::instance().sendCommand(new
			VDPDataStoreVersionCheck(*this));
	} else {
		refresh1();
	}
}

void VDPDataStore::refresh1()
{
	CommClient::instance().sendCommand(new VDPDataStoreVRAMSizeCheck(*this));
}

void VDPDataStore::refresh2()
{
	QString req =
		"debug_bin2hex "
		"[debug read_block {" + QString::fromStdString(*debuggableNameVRAM) + "} 0 " + QString::number(vramSize) + "]"
		"[debug read_block {VDP palette} 0 32]"
		"[debug read_block {VDP status regs} 0 16]"
		"[debug read_block {VDP regs} 0 64]"
		"[debug read_block {VRAM pointer} 0 2]";
	new SimpleHexRequest(req, MAX_TOTAL_SIZE - MAX_VRAM_SIZE + vramSize, &vram[0], *this);
}

void VDPDataStore::DataHexRequestReceived()
{
	//update MSXPalette that contains VDP palette
	palettes[0].setPalette(&vram[vramSize]);
	palettes[0].syncToMSX = true; //NO AUTOSYNC UNTIL WE GET THE PALETTE!

	emit dataRefreshed();
}

const uint8_t* VDPDataStore::getVramPointer() const
{
	return &vram[0];
}
const uint8_t* VDPDataStore::getPalettePointer() const
{
	return &vram[vramSize];
}
const uint8_t* VDPDataStore::getStatusRegsPointer() const
{
	return &vram[vramSize + 32];
}
const uint8_t* VDPDataStore::getRegsPointer() const
{
	return &vram[vramSize + 32 + 16];
}
const uint8_t* VDPDataStore::getVdpVramPointer() const
{
    return &vram[vramSize + 32 + 16 + 64];
}

MSXPalette* VDPDataStore::getPalette(int index)
{
    return &palettes[index];
}

size_t VDPDataStore::getVRAMSize() const
{
	return vramSize;
}
