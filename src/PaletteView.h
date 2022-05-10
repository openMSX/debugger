#ifndef PALETTEVIEW_H
#define PALETTEVIEW_H

#include "ui_PaletteView.h"
#include <QDialog>
#include <cstdint>
#include <memory>
#include <QSignalMapper>
#include "MSXPalette.h"

class QAbstractButton;

namespace Ui {
class PaletteView;
}

class PaletteView : public QWidget
{
    Q_OBJECT

public:
    explicit PaletteView(QWidget *parent = nullptr);

    void setPalette(MSXPalette* sourcePal);
    MSXPalette* getPalette();

    void syncToSource();
    void setAutoSync(bool value);

public slots:
    void refresh();

signals:
    void paletteReplaced(MSXPalette* sourcePal);
    void paletteChanged();
    void paletteSynced();

private slots:
    void colorSelected(int colorNumber);
    void on_horizontalSlider_R_valueChanged(int value);
    void on_horizontalSlider_G_valueChanged(int value);
    void on_horizontalSlider_B_valueChanged(int value);
    void restorePalette();

    void on_buttonBox_clicked(QAbstractButton* button);

    void on_cb_autosync_stateChanged(int arg1);
    void on_cbPalette_currentIndexChanged(int index);
    void on_pbCopyPaletteVDP_clicked();

    void updateText();



private:
    std::unique_ptr<Ui::PaletteView> ui;
    QSignalMapper* signalMapper;
    void combineRGB();

    MSXPalette* myPal;
    MSXPalette myOriginalPal;

    int currentColor = 0;

    bool autoSync = false;
    bool isDisplayUpdating=false;

};

#endif // PALETTEVIEW_H
