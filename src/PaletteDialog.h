#ifndef PALETTEDIALOG_H
#define PALETTEDIALOG_H

#include <QDialog>
#include <QSignalMapper>
#include <QPushButton>


namespace Ui {
class PaletteDialog;
}

class PalettePatch: public QPushButton
{
    Q_OBJECT
public:
    explicit PalettePatch(QWidget *parent = nullptr,int palnr=0);
//    ~PalettePatch();
//    void setColor(QRgb c);
public slots:
    void updatePaletteChanged(unsigned char* pal);
    void setHighlightTest(int colornr);

protected:
    void paintEvent(QPaintEvent * event) override;

private:
    QRgb mycolor;
    bool isSelected;
    int msxpalnr;

};


class PaletteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteDialog(QWidget *parent = nullptr);
    ~PaletteDialog();

    void setPalette(unsigned char *pal);
    unsigned char* getPalette();
    void syncToSource();
    void setAutoSync(bool value);

signals:
    void paletteChanged(unsigned char* pal);
    void paletteSynced();

private slots:
    void colorSelected(int colornumber);
    void on_horizontalSlider_R_valueChanged(int value);
    void on_horizontalSlider_G_valueChanged(int value);
    void on_horizontalSlider_B_valueChanged(int value);
    void restoreOpeningsPalette();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_cb_autosync_stateChanged(int arg1);

private:
    Ui::PaletteDialog *ui;
    QSignalMapper *signalMapper;
//    void decodepalette();
    void combineRGB();

    unsigned char* sourcepal;
    unsigned char mypal[32];
    unsigned char myoriginalpal[32];

    int currentcolor;
    QRgb msxpallet[16];

    bool autoSync;
};

#endif // PALETTEDIALOG_H
