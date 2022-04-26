#ifndef QUICKGUIDE_H
#define QUICKGUIDE_H

#include <QWidget>

namespace Ui {
class QuickGuide;
}

class QuickGuide : public QWidget
{
    Q_OBJECT

public:
    explicit QuickGuide(QWidget *parent = nullptr);
    ~QuickGuide();

private:
    Ui::QuickGuide *ui;
};

#endif // QUICKGUIDE_H
