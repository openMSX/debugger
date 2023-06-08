#ifndef VDPDATASTORE_H
#define VDPDATASTORE_H

#include "SimpleHexRequest.h"
#include <QObject>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class VDPDataStore : public QObject, public SimpleHexRequestUser
{
	Q_OBJECT
public:
	static VDPDataStore& instance();

	const uint8_t* getVramPointer() const;
	const uint8_t* getDefaultPalettePointer() const;
	const uint8_t* getPalettePointer() const;
	const uint8_t* getRegsPointer() const;
	const uint8_t* getStatusRegsPointer() const;
	const uint8_t* getVdpVramPointer() const;

	size_t getVRAMSize() const;

	bool getRegisterLatchAvailable() const { return registerLatchAvailable; }
	bool getPaletteLatchAvailable() const { return paletteLatchAvailable; }
	bool getDataLatchAvailable() const { return dataLatchAvailable; }
	bool getVramAccessStatusAvailable() const { return vramAccessStatusAvailable; }

	void refresh();

signals:
        void dataRefreshed(); // The refresh got the new data

	/** This might become handy later on, for now we only need the dataRefreshed
	 *
	void dataChanged(); //any of the contained data has changed
	void vramChanged(); //only the vram changed
	void paletteChanged(); //only the palette changed
	void regsChanged(); //only the regs changed
	void statusRegsChanged(); //only the regs changed
	*/

private:
	VDPDataStore();

	void DataHexRequestReceived() override;

	void refresh1();
	void refresh2();
	void refresh3();

private:
	std::vector<uint8_t> vram;
	size_t vramSize;

	std::optional<std::string> debuggableNameVRAM; // VRAM debuggable name

	bool registerLatchAvailable = false;
	bool paletteLatchAvailable = false;
	bool dataLatchAvailable = false;
	bool vramAccessStatusAvailable = false;

	friend class VDPDataStoreVersionCheck;
	friend class VDPDataStoreDebuggableChecks;
	friend class VDPDataStoreVRAMSizeCheck;
};

#endif // VDPDATASTORE_H
