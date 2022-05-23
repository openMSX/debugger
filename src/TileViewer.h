#ifndef TILEVIEWER_H
#define TILEVIEWER_H

#include "ui_TileViewer.h"
#include <QDialog>
#include <cstdint>

class VramTiledView;

class TileViewer : public QDialog, private Ui::TileViewer
{
    Q_OBJECT
public:
    explicit TileViewer(QWidget* parent = nullptr);

    void refresh();

private:
    void decodeVDPregs();

    void displayCharInfo(int screenX, int screenY, int character, const QString& textInfo);

    void on_cb_tilemapsource_currentIndexChanged(int index);
    void on_cb_screen_currentIndexChanged(int index);
    void on_le_nametable_textChanged(const QString& text);
    void on_le_colortable_textChanged(const QString& text);
    void on_le_patterntable_textChanged(const QString& text);
    void on_cb_color0_stateChanged(int state);
    void on_useVDPRegisters_stateChanged(int state);

    void on_useVDPPalette_stateChanged(int state);
    void on_zoomLevel_valueChanged(double d);

    void on_cb_drawgrid_stateChanged(int state);
    void on_cb_highlight_stateChanged(int state);
    void on_sp_highlight_valueChanged(int i);
    void on_sp_bordercolor_valueChanged(int i);

    void VDPDataStoreDataRefreshed();
    void highlightInfo(uint8_t character, int count);
    void update_label_characterimage();
    void imageMouseOver(int screenX, int screenY, int character);

    void on_cb_blinkcolors_stateChanged(int arg1);
    void on_cb_screenrows_currentIndexChanged(int index);

private:
    VramTiledView* imageWidget;
    QImage image4label;

    int mouseOverX = 0;
    int mouseOverY = 0;
    int mouseOverChar = 0;

};

#endif // TILEVIEWER_H
