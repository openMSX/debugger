#include "VDPDataStore.h"

class BigHexRequest : public ReadDebugBlockCommand
{
public:
	BigHexRequest(const QString& debuggable,  unsigned size,
	           unsigned char* target, VDPDataStore& viewer_)
		: ReadDebugBlockCommand(debuggable , size, target)
		, offset(0)
		, viewer(viewer_)
	{
	}

	BigHexRequest(const QString& debuggable, unsigned offset_, unsigned size,
	           unsigned char* target, VDPDataStore& viewer_)
		: ReadDebugBlockCommand(debuggable, offset_, size, target)
		, offset(offset_)
		, viewer(viewer_)
	{
	}

	virtual void replyOk(const QString& message)
	{
		copyData(message);
		viewer.hexdataTransfered(this);
	}

	virtual void cancel()
	{
		viewer.transferCancelled(this);
	}

	unsigned offset;

private:
	VDPDataStore& viewer;
};


VDPDataStore::VDPDataStore()
{
	vram = new unsigned char[0x20000 + 32 + 16 + 64];
	palette = vram + 0x20000;
	statusRegs = palette + 32;
	regs = statusRegs + 16;
	memset(vram,0x00,0x20000+32+16+64);
}

VDPDataStore::~VDPDataStore()
{
	delete[] vram;
}

void VDPDataStore::refresh()
{
	BigHexRequest* req = new BigHexRequest(
		"debug_bin2hex "
		"[ debug read_block {physical VRAM} 0 0x20000 ]"
		"[ debug read_block {VDP palette} 0 32 ]"
		"[ debug read_block {VDP status regs} 0 16 ]"
		"[ debug read_block {VDP regs} 0 64 ]"
		,  unsigned(0x20000+32+16+64), vram , *this);
	CommClient::instance().sendCommand(req);
}

void VDPDataStore::hexdataTransfered(BigHexRequest* r)
{
	delete r; // or transferCancelled(r) if that function would need to do more...
	emit dataRefreshed();
}

void VDPDataStore::transferCancelled(BigHexRequest* r)
{
	delete r;
}

unsigned char* VDPDataStore::getVramPointer()
{
	return vram;
}
unsigned char* VDPDataStore::getPalettePointer()
{
	return palette;
}
unsigned char* VDPDataStore::getRegsPointer()
{
	return regs;
}
unsigned char* VDPDataStore::getStatusRegsPointer()
{
	return statusRegs;
}

VDPDataStore& VDPDataStore::instance()
{
	static VDPDataStore oneInstance;
	return oneInstance;
}

