#pragma once

#include "Global.h"

class Overlay final : public QLabel
{
    Q_OBJECT
    Q_DISABLE_COPY(Overlay)
public:
    Overlay() = delete;
    explicit Overlay(QWidget* parent, Qt::ArrowType direction=Qt::NoArrow);

    static Qt::ArrowType invertArrow(Qt::ArrowType arrow);

    void setArrowshape(Qt::ArrowType arrow);
    Qt::ArrowType arrowshape();
    void reposition();

private:
    void makeArrowshape();

    QPolygon arrow;

    Qt::ArrowType arrowtype;

protected:
    void paintEvent(QPaintEvent *) override;
};
