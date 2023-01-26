#ifndef BITMAPVIEWER_OPENMSX_H
#define BITMAPVIEWER_OPENMSX_H

#include "ui_BitMapViewer.h"
#include <QDialog>

class VramBitMappedView;

class BitMapViewer : public QDialog, private Ui::BitMapViewer
{
	Q_OBJECT
public:
	BitMapViewer(QWidget* parent = nullptr);
	void refresh();

private:
	void decodeVDPregs();
	void setPages();
	void updateDisplayAsFrame();

	void on_screenMode_currentIndexChanged(int index);
	void on_currentPage_currentIndexChanged(int index);
	void on_linesVisible_currentIndexChanged(int index);
	void on_bgColor_valueChanged(int value);

	void on_useVDPRegisters_stateChanged(int state);

	void on_saveImageButton_clicked(bool checked);
	void on_editPaletteButton_clicked(bool checked);
	void on_useVDPPalette_stateChanged(int state);
	void on_zoomLevel_valueChanged(double d);

	void updateImagePosition(int x, int y, int color, unsigned addr, int byteValue);

	void VDPDataStoreDataRefreshed();

private:
	VramBitMappedView* imageWidget;
	unsigned pageSize;
	int screenMod;
	bool useVDP;
};

#endif /* BITMAPVIEWER_OPENMSX_H */
