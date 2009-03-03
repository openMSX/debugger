#ifndef VDPDATASTORE_H
#define VDPDATASTORE_H

#include "SimpleHexRequest.h"
#include <QObject>

class VDPDataStore : public QObject, public SimpleHexRequestUser
{
	Q_OBJECT
public:
	static VDPDataStore& instance();

	const unsigned char* getVramPointer() const;
	const unsigned char* getPalettePointer() const;
	const unsigned char* getRegsPointer() const;
	const unsigned char* getStatusRegsPointer() const;

private:
	VDPDataStore();
	~VDPDataStore();

	virtual void DataHexRequestReceived();

	unsigned char* vram;

	bool old_version; // VRAM debuggable has old or new name?
	bool got_version; // is the above boolean already filled in?
	friend class VDPDataStoreVersionCheck;

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
