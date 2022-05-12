#ifndef VDPDATASTORE_H
#define VDPDATASTORE_H

#include "MSXPalette.h"
#include "SimpleHexRequest.h"
#include <QObject>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief The VDPDataStore class is a singleton that keeps track of all VDP related data for all the viewers
 *
 * It does keep track of the VRAM, registers, palettes and statusregisters
 * For the viewers it keeps track of 4 palettes.
 * The actual openMSX palette, a palette or the tileviewers, the bitmap viewers and the spriteviewers
 */
class VDPDataStore : public QObject, public SimpleHexRequestUser
{
	Q_OBJECT
public:
	static VDPDataStore& instance();

	const uint8_t* getVramPointer() const;
	const uint8_t* getPalettePointer() const;
	const uint8_t* getRegsPointer() const;
	const uint8_t* getStatusRegsPointer() const;
	const uint8_t* getVdpVramPointer() const;

	MSXPalette* getPalette(int index);
	size_t getVRAMSize() const;

private:
	VDPDataStore();

	void DataHexRequestReceived() override;

	void refresh1();
	void refresh2();

private:
	MSXPalette palettes[4];

	std::vector<uint8_t> vram;
	size_t vramSize;

	std::optional<std::string> debuggableNameVRAM; // VRAM debuggable name

	friend class VDPDataStoreVersionCheck;
	friend class VDPDataStoreVRAMSizeCheck;

public slots:
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
};

#endif // VDPDATASTORE_H
