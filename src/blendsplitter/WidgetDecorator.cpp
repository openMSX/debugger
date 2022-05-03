#include "WidgetDecorator.h"

#include "BlendSplitter.h"
#include "ExpanderCorner.h"
#include <QPainter>
#include <QPoint>
#include <QDebug>

WidgetDecorator::WidgetDecorator(QWidget* widget)
    : widget{widget}
    //, expanderBottom{new ExpanderBottom{this}}
    //, expanderTop{new ExpanderTop{this}}
    , expanderCorner1{new ExpanderCorner{this, Qt::TopLeftCorner}}
    , expanderCorner2{new ExpanderCorner{this, Qt::BottomRightCorner}}
    , expanderCorner3{new ExpanderCorner{this, Qt::TopRightCorner}}
    , expanderCorner4{new ExpanderCorner{this, Qt::BottomLeftCorner}}
    , dropzone(dropregions::center)
{
    auto* layout = new QHBoxLayout{};
    layout->addWidget(widget);
    layout->setMargin(0);
    setLayout(layout);
    setMinimumSize(2 * BlendSplitter::expanderSize, 2 * BlendSplitter::expanderSize);
}

WidgetDecorator::~WidgetDecorator()
{
    delete layout();
}

void WidgetDecorator::resizeEvent(QResizeEvent*)
{
    //expanderBottom->reposition();
    //expanderTop->reposition();
    expanderCorner1->reposition();
    expanderCorner2->reposition();
    expanderCorner3->reposition();
    expanderCorner4->reposition();
}

//void WidgetDecorator::mouseMoveEvent(QMouseEvent* event)
//{
//    determineDropZone(event->pos());
//    update();
//}

void WidgetDecorator::determineDropZone(QPoint pos)
{
    int x = width()  / 3;
    int y = height() / 3;
    if (pos.x() > x && pos.x() < (width() - x) && pos.y() > y && pos.y() < (height() - y)) {
        dropzone = dropregions::center;
        return;
    }
    // now if we are not center then use the diagonals of the rect to see in which of the 4 triangles we are

    //normal on diagonal (0, 0) -> (width, height)
    x = -height();
    y = width();
    //we use the sign of the dot product to determine if we are above/below the diagonal
    int side = (x * pos.x() + y * pos.y()) > 0 ? 1 : 0;

    //normal on diagonal (0, height) -> (width, 0) == (0, 0)-(width, -height)
    x = height();
    y = width();
    //we use the sign of the dot product to determine if we are above/below the diagonal
    side += (x * pos.x() + y * (pos.y() - height())) > 0 ? 2 : 0;
    dropzone = static_cast<dropregions>(side);
}

//void WidgetDecorator::paintEvent(QPaintEvent* event)
//{
//    QWidget::paintEvent(event);
//
//    //qDebug() << "WidgetDecorator::paintEvent";
//
//    QPainter painter(this);
//    painter.setPen(Qt::black);
//    painter.setBrush(Qt::NoBrush);
//    int x = width()  / 3;
//    int y = height() / 3;
//    painter.drawLine(0, 0, x, y);
//    painter.drawLine(width(), 0, width() - x, y);
//    painter.drawLine(0, height(), x, height() - y);
//    painter.drawLine(width(), height(), width() - x, height() - y);
//    painter.drawRect(x, y, x, y);
//
//    switch (dropzone) {
//    case dropregions::top:
//        painter.drawText(QRect(x, 0, x, y), Qt::AlignCenter, "top");
//        break;
//    case dropregions::left:
//        painter.drawText(QRect(0, y, x, y), Qt::AlignCenter, "left");
//        break;
//    case dropregions::right:
//        painter.drawText(QRect(width()-x, y, x, y), Qt::AlignCenter, "right");
//        break;
//    case dropregions::bottom:
//        painter.drawText(QRect(x, height()-y, x, y), Qt::AlignCenter, "bottom");
//        break;
//    case dropregions::center:
//        painter.drawText(QRect(x, y, x, y), Qt::AlignCenter, "center");
//        break;
//    default:
//        break;
//    }
//    setMouseTracking(true);
//}
