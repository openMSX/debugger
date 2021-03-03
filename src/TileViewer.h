#ifndef TILEVIEWER_H
#define TILEVIEWER_H

#include "ui_TileViewer.h"
#include <QDialog>

class VramTiledView; 

class TileViewer : public QDialog,private Ui::TileViewer
{
    Q_OBJECT

public:
    explicit TileViewer(QWidget *parent = nullptr);
    ~TileViewer();

private:
    void decodeVDPregs();

    VramTiledView* imageWidget;
    QImage image4label;

private slots:
    void refresh();

    void characterSelected2Text(int screenx, int screeny, int character, QString textinfo);

    void on_cb_tilemapsource_currentIndexChanged(int index);
    void on_cb_screen_currentIndexChanged(int index);
    void on_le_nametable_textChanged(const QString &text);
    void on_le_colortable_textChanged(const QString &text);
    void on_le_patterntable_textChanged(const QString &text);
    void on_cb_color0_stateChanged(int state);
    void on_useVDPRegisters_stateChanged(int state);

    void on_editPaletteButton_clicked(bool checked);
    void on_useVDPPalette_stateChanged(int state);
    void on_zoomLevel_valueChanged(double d);

    void on_cb_drawgrid_stateChanged(int state);
    void on_cb_highlight_stateChanged(int state);
    void on_sp_highlight_valueChanged(int i);
    void on_sp_bordercolor_valueChanged(int i);



    void VDPDataStoreDataRefreshed();
    void highlightInfo(unsigned char character, int count);
    void imageMouseOver(int screenx, int screeny, int character);

    void on_cb_blinkcolors_stateChanged(int arg1);
    void on_cb_screenrows_currentIndexChanged(int index);

};

#endif // TILEVIEWER_H
