#include "VDPDataStore.h"
#include "CommClient.h"

// static vector to feed PaletteDialog and be used when VDP colors aren't selected
static const uint8_t defaultPalette[32] = {
//    RB  G
        0x00, 0,
        0x00, 0,
        0x11, 6,
        0x33, 7,
        0x17, 1,
        0x27, 3,
        0x51, 1,
        0x27, 6,
        0x71, 1,
        0x73, 3,
        0x61, 6,
        0x64, 6,
        0x11, 4,
        0x65, 2,
        0x55, 5,
        0x77, 7,
};

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

class VDPDataStoreDebuggableChecks : public SimpleCommand
{
public:
	VDPDataStoreDebuggableChecks(VDPDataStore& dataStore_)
		: SimpleCommand("debug_check_debuggables {{VDP register latch status} {VDP palette latch status} "
		                "{VDP data latch value} {VRAM access status}}")
		, dataStore(dataStore_)
	{
	}

	void replyOk(const QString& message) override
	{
		assert(message.size() == 7);
		QStringList s = message.split(' ');
		dataStore.registerLatchAvailable = s[0] == '1';
		dataStore.paletteLatchAvailable = s[1] == '1';
		dataStore.dataLatchAvailable = s[2] == '1';
		dataStore.vramAccessStatusAvailable = s[3] == '1';
		dataStore.refresh3();
		delete this;
	}
	void replyNok(const QString& /*message*/) override
	{
		dataStore.refresh3();
		delete this;
	}

private:
	VDPDataStore& dataStore;
};

static constexpr unsigned MAX_VRAM_SIZE = 0x30000;
static constexpr unsigned MAX_TOTAL_SIZE = MAX_VRAM_SIZE + 32 + 16 + 64 + 2 + 3 + 1;

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
	CommClient::instance().sendCommand(new VDPDataStoreDebuggableChecks(*this));
}

void VDPDataStore::refresh3()
{
	QString req = QString(
		"debug_bin2hex "
		"[debug read_block {" + QString::fromStdString(*debuggableNameVRAM) + "} 0 " + QString::number(vramSize) + "]"
		"[debug read_block {VDP palette} 0 32]"
		"[debug read_block {VDP status regs} 0 16]"
		"[debug read_block {VDP regs} 0 64]"
		"[debug read_block {VRAM pointer} 0 2]%1%2%3%4")
		.arg(registerLatchAvailable ? "[debug read_block {VDP register latch status} 0 1]" : "")
		.arg(paletteLatchAvailable ? "[debug read_block {VDP palette latch status} 0 1]" : "")
		.arg(dataLatchAvailable ? "[debug read_block {VDP data latch value} 0 1]" : "")
		.arg(vramAccessStatusAvailable ? "[debug read_block {VRAM access status} 0 1]" : "");

	int total = MAX_TOTAL_SIZE - MAX_VRAM_SIZE + vramSize - !registerLatchAvailable
		- !paletteLatchAvailable - !dataLatchAvailable - !vramAccessStatusAvailable;
	new SimpleHexRequest(req, total, &vram[0], *this);
}

void VDPDataStore::DataHexRequestReceived()
{
	emit dataRefreshed();
}

const uint8_t* VDPDataStore::getVramPointer() const
{
	return &vram[0];
}
const uint8_t* VDPDataStore::getDefaultPalettePointer() const
{
	return defaultPalette;
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

size_t VDPDataStore::getVRAMSize() const
{
	return vramSize;
}
