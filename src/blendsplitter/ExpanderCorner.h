#pragma once

#include "Global.h"

#include "Expander.h"
class WidgetDecorator;
class BlendSplitter;

class ExpanderCorner final : public Expander
{
    Q_OBJECT
    Q_DISABLE_COPY(ExpanderCorner)
public:
    ExpanderCorner() = delete;
    explicit ExpanderCorner(WidgetDecorator* parent,Qt::Corner location);
    virtual void reposition() override;
protected slots:
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override final;
    virtual void mouseReleaseEvent(QMouseEvent* event) override final;

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

    void decideDragAction(QMouseEvent *event, WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter);
    void performInnerSplit(WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter, Qt::Orientation splitorientation);
    void setupJoiners(WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter, int x, int y, Qt::Orientation splitorientation);
    void followDragJoiners(WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter, int x, int y, Qt::Orientation splitorientation);
    bool isInContinuationOfSplitter(BlendSplitter *parentSplitter, int x, int y);
    bool isOnTrailingHandler(BlendSplitter *parentSplitter);

    Overlay* internalOverlay;
    Overlay* externalOverlay;
    QWidget* externalJoinWidget;
    Qt::ArrowType joinarrow;
    int pickCoordinate(int x,int y,Qt::Orientation orient);
    int pickSize(const QSize &size, Qt::Orientation orient);
};
