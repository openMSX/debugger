#ifndef BITMAPVIEWER_H
#define BITMAPVIEWER_H

#include <QDialog>
#include <QColor>
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"
#include "VramBitMappedView.h"
#include "ui_BitMapViewer.h"

class HexRequest;

class BitMapViewer : public QDialog, private Ui::BitMapViewer
{
	Q_OBJECT
public:
	BitMapViewer( QWidget *parent = 0);
	~BitMapViewer();

	/*
	void hexdataTransfered(HexRequest* r);
	void transferCancelled(HexRequest* r);
	*/

private:
	int screenMod;
	int lines;
	int showpage;
	int borderColor;
	bool useVDP;
	bool useVDPcolors;

	int vramAddress;
	int vramSize;
	unsigned char* vram;
	unsigned char* palette;
	unsigned char* regs;

	void setDefaultPalette();
	void decodeVDPregs();

	VramBitMappedView *imageWidget;

private slots:
	void refresh();

	void on_screenMode_currentIndexChanged( const QString & text );
	void on_showPage_currentIndexChanged( int index );
	void on_linesVisible_currentIndexChanged( int index );
	void on_bgColor_valueChanged ( int value );

	void on_useVDPRegisters_stateChanged ( int state );

	void on_saveImageButton_clicked ( bool checked );
	void on_editPaletteButton_clicked ( bool checked );
	void on_useVDPPalette_stateChanged ( int state );
	void on_zoomLevel_valueChanged ( double d );

	void imagePositionUpdate(int x, int y, int color, unsigned addr,int byteValue);

	//void on_refreshButton_clicked ( bool checked );
	void on_VDPDataStore_dataRefreshed();
};

#endif /* BITMAPVIEWER_H */
