#ifndef WIDGETDECORATOR_H
#define WIDGETDECORATOR_H

#include "Global.h"

class BlendSplitter;
class ExpanderCorner;

class WidgetDecorator final : public QWidget
{
    Q_OBJECT

public:
    WidgetDecorator(const WidgetDecorator&) = delete;
    WidgetDecorator& operator=(const WidgetDecorator&) = delete;
    explicit WidgetDecorator(QWidget* widget);
    ~WidgetDecorator();

private:
    void determineDropZone(QPoint pos);

private:
    QWidget* widget;
    ExpanderCorner* expanderCorner1;
    ExpanderCorner* expanderCorner2;
    ExpanderCorner* expanderCorner3;
    ExpanderCorner* expanderCorner4;

    enum class dropregions {
        top = 0,
        left = 1,
        right = 2,
        bottom = 3,
        center = 4
    };
    dropregions dropzone;

protected slots:
    void resizeEvent(QResizeEvent*) override;
    //void mouseMoveEvent(QMouseEvent* event) override;
    //void paintEvent(QPaintEvent* event) override;
};

#endif
