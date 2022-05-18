#ifndef EXPANDERCORNER_H
#define EXPANDERCORNER_H

#include "Global.h"

#include "Expander.h"

class WidgetDecorator;
class BlendSplitter;

class ExpanderCorner final : public Expander
{
    Q_OBJECT

public:
    ExpanderCorner(const ExpanderCorner&) = delete;
    ExpanderCorner& operator=(const ExpanderCorner&) = delete;
    explicit ExpanderCorner(WidgetDecorator* parent, Qt::Corner location);
    void reposition() override;

protected slots:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

private:
    void decideDragAction(QMouseEvent* event, WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter);
    void performInnerSplit(WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter, Qt::Orientation splitorientation);
    void setupJoiners(WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter, int x, int y, Qt::Orientation splitorientation);
    void followDragJoiners(WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter, int x, int y, Qt::Orientation splitorientation);
    bool isInContinuationOfSplitter(BlendSplitter* parentSplitter, int x, int y);
    bool isOnTrailingHandler(BlendSplitter* parentSplitter);

    int pickCoordinate(int x, int y, Qt::Orientation orient);
    int pickSize(const QSize &size, Qt::Orientation orient);

private:
    Qt::Corner corner;
    //next variables are used to see if we have mouseMovements inwards or outwards of our decorating widget
    int unitX; // x-step to centrum of WidgetDecorator (1 or -1)
    int unitY; // y-step to centrum of WidgetDecorator (1 or -1)
    int hotspotX;
    int hotspotY;
    enum {
        undecidedDrag,
        joinDrag,
        splitDrag,
    } dragaction;
    Qt::Orientation dragorientation;

    Overlay* internalOverlay;
    Overlay* externalOverlay;
    Qt::ArrowType joinarrow;

protected slots:
    void enterEvent(QEvent* event) final;
};

#endif