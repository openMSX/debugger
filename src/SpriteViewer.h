#ifndef SPRITEVIEWER_H
#define SPRITEVIEWER_H

#include <QDialog>
#include <cstdint>

namespace Ui {
class SpriteViewer;
}

class VramSpriteView;

class SpriteViewer : public QDialog
{
    Q_OBJECT

public:
    explicit SpriteViewer(QWidget* parent = nullptr);
    ~SpriteViewer();

private slots:
    void refresh();
    void VDPDataStoreDataRefreshed();
    void pgtwidget_mouseMoveEvent(int x, int y, int character);
    void pgtwidget_mouseClickedEvent(int x, int y, int character, QString text);
    void spatwidget_mouseMoveEvent(int x, int y, int character);
    void spatwidget_mouseClickedEvent(int x, int y, int character, QString text);

    void setDrawGrid(int state);

    void on_le_patterntable_textChanged(const QString& arg1);
    void on_le_attributentable_textChanged(const QString  &arg1);
    void on_useVDPRegisters_toggled(bool checked);
    void on_cb_size_currentIndexChanged(int index);
    void on_cb_spritemode_currentIndexChanged(int index);
    void on_le_colortable_textChanged(const QString& arg1);
    void on_sp_zoom_valueChanged(int arg1);
    void on_cb_ecinfluence_toggled(bool checked);
    void on_cb_mag_currentIndexChanged(int index);
    void on_cb_alwaysShowColorTable_toggled(bool checked);
    void on_useVDPPalette_stateChanged(int state);
    void on_editPaletteButton_clicked(bool checked);

private:
    static uint8_t defaultPalette[32];
    void setPaletteSource(const uint8_t* palsource, bool useVDP);

    void decodeVDPregs();

    Ui::SpriteViewer* ui;
    VramSpriteView* imageWidget;
    VramSpriteView* imageWidgetSingle;
    VramSpriteView* imageWidgetSpat;
    VramSpriteView* imageWidgetColor;

    int spriteMode = 0;
    bool size16x16 = false;
    bool magnified = false;
    bool spritesEnabled = false;
    int spAtAddr = 0;
    int pgtAddr = 0;
    int spColAddr = 0;

    void setCorrectEnabled(bool checked);
    void setCorrectVDPData();
};

#endif // SPRITEVIEWER_H
