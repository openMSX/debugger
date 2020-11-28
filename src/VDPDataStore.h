#ifndef VDPDATASTORE_H
#define VDPDATASTORE_H

#include "SimpleHexRequest.h"
#include <QObject>
#include <string>

class VDPDataStore : public QObject, public SimpleHexRequestUser
{
	Q_OBJECT
public:
	static VDPDataStore& instance();

	const unsigned char* getVramPointer() const;
	const unsigned char* getPalettePointer() const;
	const unsigned char* getRegsPointer() const;
	const unsigned char* getStatusRegsPointer() const;
	const unsigned char* getVdpVramPointer() const;

	size_t getVRAMSize() const;

private:
	VDPDataStore();
	~VDPDataStore() override;

	void DataHexRequestReceived() override;

	void refresh1();
	void refresh2();

	unsigned char* vram;
	size_t vramSize;

	std::string debuggableNameVRAM; // VRAM debuggable name
	bool got_version; // is the above boolean already filled in?
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

#endif /* VDPDATASTORE_H */
