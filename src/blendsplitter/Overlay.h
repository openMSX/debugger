#ifndef OVERLAY_H
#define OVERLAY_H

#include "Global.h"

class Overlay final : public QLabel
{
    Q_OBJECT

public:
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;
    explicit Overlay(QWidget* parent, Qt::ArrowType direction = Qt::NoArrow);

    static Qt::ArrowType invertArrow(Qt::ArrowType arrow);

    void setArrowshape(Qt::ArrowType arrow);
    Qt::ArrowType arrowshape();
    void reposition();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void makeArrowshape();

private:
    QPolygon arrow;
    Qt::ArrowType arrowtype;
};

#endif
