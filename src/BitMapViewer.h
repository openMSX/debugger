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

private:
	void decodeVDPregs();
	void setPages();

	VramBitMappedView* imageWidget;
	int screenMod;
	bool useVDP;

private slots:
	void refresh();

	void on_screenMode_currentIndexChanged(const QString& text);
	void on_showPage_currentIndexChanged(int index);
	void on_linesVisible_currentIndexChanged(int index);
	void on_bgColor_valueChanged(int value);

	void on_useVDPRegisters_stateChanged(int state);

	void on_saveImageButton_clicked(bool checked);
	void on_editPaletteButton_clicked(bool checked);
	void on_useVDPPalette_stateChanged(int state);
	void on_zoomLevel_valueChanged(double d);

	void updateImagePosition(int x, int y, int color, unsigned addr, int byteValue);

	void VDPDataStoreDataRefreshed();
};

#endif /* BITMAPVIEWER_OPENMSX_H */
