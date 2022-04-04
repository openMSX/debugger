#ifndef PALETTEDIALOG_H
#define PALETTEDIALOG_H

#include "ui_PaletteDialog.h"
#include <QDialog>
#include <QSignalMapper>
#include <QPushButton>
#include <cstdint>
#include <memory>

namespace Ui {
    class PaletteDialog;
}

class PalettePatch: public QPushButton
{
    Q_OBJECT
public:
    explicit PalettePatch(QWidget* parent = nullptr, int palNr = 0);
    //void setColor(QRgb c);
public slots:
    void updatePaletteChanged(const uint8_t* pal);
    void setHighlightTest(int colorNr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRgb myColor = Qt::green;
    bool isSelected = false;
    int msxPalNr;
};


class PaletteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteDialog(QWidget* parent = nullptr);

    void setPalette(uint8_t* pal);
    uint8_t* getPalette();
    void syncToSource();
    void setAutoSync(bool value);

signals:
    void paletteChanged(uint8_t* pal);
    void paletteSynced();

private slots:
    void colorSelected(int colorNumber);
    void on_horizontalSlider_R_valueChanged(int value);
    void on_horizontalSlider_G_valueChanged(int value);
    void on_horizontalSlider_B_valueChanged(int value);
    void restoreOpeningsPalette();

    void on_buttonBox_clicked(QAbstractButton* button);

    void on_cb_autosync_stateChanged(int arg1);

    void updateText();

private:
    std::unique_ptr<Ui::PaletteDialog> ui;
    QSignalMapper* signalMapper;
    //void decodepalette();
    void combineRGB();

    uint8_t* sourcePal = nullptr;
    uint8_t myPal[32];
    uint8_t myOriginalPal[32];

    int currentColor = 0;
    QRgb msxPalette[16];

    bool autoSync = false;
};

#endif // PALETTEDIALOG_H
