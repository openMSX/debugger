#ifndef SPRITEVIEWER_H
#define SPRITEVIEWER_H

#include <QDialog>

namespace Ui {
class SpriteViewer;
}

class VramSpriteView;

class SpriteViewer : public QDialog
{
    Q_OBJECT

public:
    explicit SpriteViewer(QWidget *parent = nullptr);
    ~SpriteViewer();

private slots:
    void refresh();
    void VDPDataStoreDataRefreshed();
    void pgtwidget_mouseMoveEvent(int x, int y, int character);
    void pgtwidget_mouseClickedEvent(int x, int y, int character,QString text);
    void spatwidget_mouseMoveEvent(int x, int y, int character);
    void spatwidget_mouseClickedEvent(int x, int y, int character,QString text);

    void setDrawGrid(int state);

    void on_le_patterntable_textChanged(const QString &arg1);

    void on_le_attributentable_textChanged(const QString &arg1);

    void on_useVDPRegisters_toggled(bool checked);

    void on_cb_size_currentIndexChanged(int index);

    void on_cb_spritemode_currentIndexChanged(int index);

    void on_le_colortable_textChanged(const QString &arg1);

private:
    void decodeVDPregs();

    Ui::SpriteViewer *ui;
    VramSpriteView* imageWidget;
    VramSpriteView* imageWidgetSingle;
    VramSpriteView* imageWidgetSpat;

    int spritemode;
    bool size16x16;
    bool magnified;
    bool spritesenabled;
    int spataddr;
    int pgtaddr;
    int spcoladdr;

    void setCorrectEnabled(bool checked);
    void setCorrectVDPData();
};

#endif // SPRITEVIEWER_H
