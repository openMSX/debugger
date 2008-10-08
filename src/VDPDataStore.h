#ifndef VDPDATASTORE_H
#define VDPDATASTORE_H

#include <QObject>
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"

class BigHexRequest;

class VDPDataStore : public QObject
{
	Q_OBJECT
public:
	VDPDataStore();
	~VDPDataStore();

	static VDPDataStore& instance();

	void hexdataTransfered(BigHexRequest* r);
	void transferCancelled(BigHexRequest* r);
	unsigned char* getVramPointer();
	unsigned char* getPalettePointer();
	unsigned char* getRegsPointer();
	unsigned char* getStatusRegsPointer();

private:
	unsigned char* vram;
	unsigned char* palette;
	unsigned char* regs;
	unsigned char* statusRegs;


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
