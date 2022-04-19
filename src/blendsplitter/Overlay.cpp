#include "Overlay.h"
#include <QPainter>
#include <QColor>
#include <QPolygon>

Overlay::Overlay(QWidget* parent, Qt::ArrowType direction) : QLabel(parent),arrow(),arrowtype(direction)
{
    move(0, 0);
    reposition();
    //    setStyleSheet("background-color: rgba(0, 0, 0, 128);");
}

void Overlay::setArrowshape(Qt::ArrowType arrow)
{
    arrowtype=arrow;
    //makeArrowshape();
    //update();
}

Qt::ArrowType Overlay::arrowshape()
{
    return arrowtype;
}

Qt::ArrowType Overlay::invertArrow(Qt::ArrowType arrow)
{
    switch (arrow) {
    case Qt::UpArrow:
        return Qt::DownArrow;
        break;
    case Qt::DownArrow:
        return Qt::UpArrow;
        break;
    case Qt::LeftArrow:
        return Qt::RightArrow;
        break;
    case Qt::RightArrow:
        return Qt::LeftArrow;
        break;
    case Qt::NoArrow:
    default:
        return Qt::NoArrow;
        break;
    }
}

void Overlay::reposition()
{
    resize(parentWidget()->width(), parentWidget()->height());
    raise();
    makeArrowshape();
    update();
}

void Overlay::makeArrowshape()
{
    if (arrowtype == Qt::NoArrow){
        arrow.clear();
        return;
    }

    int unit,midpoint;
    int height=parentWidget()->height();
    int width=parentWidget()->width();

    switch(arrowtype){
    case Qt::DownArrow:
    case Qt::UpArrow:
        //create vertical pointing arrow
        midpoint=width/2;
        unit=std::min(midpoint , height/2) /4;
        break;
    case Qt::LeftArrow:
    case Qt::RightArrow:
        //create horizontal pointing arrow
        midpoint=height/2;
        unit=std::min(midpoint , width/2) /4;
    case Qt::NoArrow:
    default:
        break;
    }

    switch(arrowtype){
    case Qt::DownArrow:
        arrow.setPoints(7,
                        midpoint+unit,0,
                        midpoint+unit,2*unit,
                        midpoint+2*unit,2*unit,
                        midpoint, 4*unit,
                        midpoint-2*unit,2*unit,
                        midpoint-unit,2*unit,
                        midpoint-unit,0
                        );
        break;
    case Qt::UpArrow:
        arrow.setPoints(7,
                        midpoint+unit,height,
                        midpoint+unit,height-2*unit,
                        midpoint+2*unit,height-2*unit,
                        midpoint, height-4*unit,
                        midpoint-2*unit,height-2*unit,
                        midpoint-unit,height-2*unit,
                        midpoint-unit,height
                        );
        break;
    case Qt::RightArrow:
        arrow.setPoints(7,
                        0,midpoint+unit,
                        2*unit,midpoint+unit,
                        2*unit,midpoint+2*unit,
                        4*unit,midpoint,
                        2*unit,midpoint-2*unit,
                        2*unit,midpoint-unit,
                        0,midpoint-unit
                        );
        break;
    case Qt::LeftArrow:
        arrow.setPoints(7,
                        width,midpoint+unit,
                        width-2*unit,midpoint+unit,
                        width-2*unit,midpoint+2*unit,
                        width-4*unit,midpoint,
                        width-2*unit,midpoint-2*unit,
                        width-2*unit,midpoint-unit,
                        width,midpoint-unit
                        );
    case Qt::NoArrow:
    default:
        break;
     };


}

void Overlay::paintEvent(QPaintEvent *)
{
    QPainter qp(this);
    qp.setBrush(QColor(0,0,0,128));
    qp.setPen(Qt::NoPen);
    qp.drawRect(0,0,size().width(),size().height() );
    qp.setBrush(QColor(255,255,255,128));
    qp.drawPolygon(arrow);
}
