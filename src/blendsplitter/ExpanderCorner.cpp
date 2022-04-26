#include "ExpanderCorner.h"

#include "BlendSplitter.h"
#include "Overlay.h"
#include "WidgetDecorator.h"
#include "SplitterDecorator.h"

#include "SignalDispatcher.h"

#include <QDebug>

ExpanderCorner::ExpanderCorner(WidgetDecorator* parent,Qt::Corner location) : Expander{parent} ,
    corner{location},unitX{1},unitY{1},hotspotX{0},hotspotY{0},dragaction{undecidedDrag},
    dragorientation{Qt::Horizontal},internalOverlay{nullptr},externalOverlay{nullptr},
    joinarrow{Qt::NoArrow}
{
    //now do some masking and pixmap rotating depending on location
    //also set out unit steps
    QPolygon mask;
    switch (location) {
    case Qt::TopLeftCorner: {
        QTransform rot;
        *pixmap = pixmap->transformed(rot.rotate(-90), Qt::FastTransformation);
        setPixmap(*pixmap);
        mask << QPoint{BlendSplitter::expanderSize, 0}
             << QPoint{BlendSplitter::expanderSize, BlendSplitter::expanderSize/10}
             << QPoint{BlendSplitter::expanderSize / 10, BlendSplitter::expanderSize}
             << QPoint{0, BlendSplitter::expanderSize}
             << QPoint{0, 0};
// done by constructor unitX=1;
// done by constructor unitY=1;
            };
        break;
    case Qt::TopRightCorner: {
        QTransform rot;
        *pixmap = pixmap->transformed(rot.rotate(0), Qt::FastTransformation);
        setPixmap(*pixmap);
        mask << QPoint{0, 0}
             << QPoint{0, BlendSplitter::expanderSize/10}
             << QPoint{BlendSplitter::expanderSize * 9 / 10, BlendSplitter::expanderSize}
             << QPoint{BlendSplitter::expanderSize, BlendSplitter::expanderSize}
             << QPoint{BlendSplitter::expanderSize, 0};
        unitX=-1;
// done by constructor unitY=1;
        hotspotX=BlendSplitter::expanderSize-1;
        };
        break;
    case Qt::BottomLeftCorner: {
        QTransform rot;
        *pixmap = pixmap->transformed(rot.rotate(180), Qt::FastTransformation);
        setPixmap(*pixmap);
        mask << QPoint{0, 0}
             << QPoint{BlendSplitter::expanderSize/10, 0}
             << QPoint{BlendSplitter::expanderSize, BlendSplitter::expanderSize * 9 / 10}
             << QPoint{BlendSplitter::expanderSize, BlendSplitter::expanderSize}
             << QPoint{0, BlendSplitter::expanderSize};
// done by constructor unitX=1;
        unitY=-1;
        hotspotY=BlendSplitter::expanderSize-1;
        };
        break;
    case Qt::BottomRightCorner: {
        QTransform rot;
        *pixmap = pixmap->transformed(rot.rotate(90), Qt::FastTransformation);
        setPixmap(*pixmap);
        mask << QPoint{BlendSplitter::expanderSize, 0}
             << QPoint{BlendSplitter::expanderSize * 9 /10, 0}
             << QPoint{0, BlendSplitter::expanderSize * 9 / 10}
             << QPoint{0, BlendSplitter::expanderSize}
             << QPoint{BlendSplitter::expanderSize, BlendSplitter::expanderSize};
        };
        unitX=-1;
        unitY=-1;
        hotspotX=BlendSplitter::expanderSize-1;
        hotspotY=BlendSplitter::expanderSize-1;
    default:
        break;

    }
    setMask(QRegion{mask});
}

/**
 * @brief ExpanderCorner::reposition
 *
 * If the parent WidgetDecorator/SplitterDecorator gets resized, this method
 * makes sure that we end up in the new correct location.
 *
 */
void ExpanderCorner::reposition()
{
    switch (corner) {
    case Qt::TopLeftCorner:
        move(0, 0);
        break;
    case Qt::TopRightCorner:
        move(parentWidget()->width() - width(), 0);
        break;
    case Qt::BottomLeftCorner:
        move(0, parentWidget()->height()-height());
        break;
    case Qt::BottomRightCorner:
        move(parentWidget()->width() - width(), parentWidget()->height()-height());
        break;
    default:
        break;
    }
    raise();
}

bool ExpanderCorner::isOnTrailingHandler(BlendSplitter* parentSplitter)
{
    return (parentSplitter->orientation()==Qt::Horizontal)?  unitX<0 : unitY<0;
}

/**
 * @brief ExpanderCorner::performInnerSplit
 * @param parentDecorator
 * @param parentSplitter
 * @param splitorientation
 *
 * This will split the widget.
 * If the orientation of the split corresponds to the splitter we simply add an item
 * If the orietation is orthogonal we replace the widget with a new splitter and have the widget added to the new splitter
 */
void ExpanderCorner::performInnerSplit(WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter, Qt::Orientation splitorientation)
{
    SwitchingWidget* switchwdg=dynamic_cast<SwitchingWidget*>(parentDecorator->layout()->itemAt(0)->widget());
    SwitchingWidget* addedWidget=nullptr;

    if (parentSplitter->orientation() == splitorientation){

        QList<int> sizes{parentSplitter->sizes()};
        int index{parentSplitter->indexOf(parentDecorator)};
        if (!isOnTrailingHandler(parentSplitter)){
            sizes.insert(index, BlendSplitter::expanderSize);
            sizes[index + 1] -= BlendSplitter::expanderSize + 1;
            parentSplitter->insertWidget(index);
            addedWidget=dynamic_cast<SwitchingWidget*>(parentSplitter->getNestedWidget(index));
        } else {
            sizes.insert(index +1, BlendSplitter::expanderSize);
            sizes[index] -= BlendSplitter::expanderSize + 1;
            parentSplitter->insertWidget(index+1);
            addedWidget=dynamic_cast<SwitchingWidget*>(parentSplitter->getNestedWidget(index+1));
        }
        parentSplitter->setSizes(sizes);
        parentSplitter->handle(index + 1)->grabMouse();
    } else {
        //add a new splitter orthogonal to the current one
        Qt::Orientation newOrientation{parentSplitter->orientation()==Qt::Horizontal?Qt::Vertical:Qt::Horizontal};
        BlendSplitter* newSplitter{new BlendSplitter{parentSplitter->defaultWidget, newOrientation}};
        QList<int> sizes{parentSplitter->sizes()};
        parentSplitter->insertSplitter(parentSplitter->indexOf(parentDecorator), newSplitter);
        // add a widget in current splitter but on the correct side
        bool after = (newOrientation==Qt::Horizontal)? unitX>0 : unitY>0;

        if (after){
            newSplitter->addWidget();
            newSplitter->addDecoratedWidget(parentDecorator);
            addedWidget=dynamic_cast<SwitchingWidget*>(newSplitter->getNestedWidget(0));
        } else {
            newSplitter->addDecoratedWidget(parentDecorator);
            newSplitter->addWidget();
            addedWidget=dynamic_cast<SwitchingWidget*>(newSplitter->getNestedWidget(1));
        }
        parentSplitter->setSizes(sizes);
        newSplitter->handle(1)->grabMouse();
   }

   //now if the original item was a SwitchingWidget we set the same and the enablestate
   if (switchwdg && addedWidget){
        addedWidget->setEnableWidget(switchwdg->getEnableWidget());
        //if new user we need show the quickguide (is the default widget) otherwise
//        addedWidget->setCurrentIndex(switchwdg->getCurrentIndex());
   }

    //have the cursor take the correct shape
   setCursor(splitorientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);

}

void ExpanderCorner::setupJoiners(WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter, int x, int y, Qt::Orientation /*splitorientation*/)
{
    Q_ASSERT(dragaction ==undecidedDrag);
    Q_ASSERT(externalOverlay == nullptr);
    Q_ASSERT(internalOverlay == nullptr);
    if(isInContinuationOfSplitter(parentSplitter,pos().x()+x,pos().y()+y) ){
        QWidget* wdgt = nullptr;
        if( isOnTrailingHandler(parentSplitter) and
                parentSplitter->indexOf(parentDecorator) + 1 < parentSplitter->count()) {

            wdgt = parentSplitter->widget(parentSplitter->indexOf(parentDecorator) + 1);
            //do not join if the target widget is also splitted
            if (!wdgt->inherits("SplitterDecorator")){
                joinarrow=parentSplitter->orientation()==Qt::Horizontal?Qt::RightArrow:Qt::DownArrow;
                externalOverlay = new Overlay{wdgt,joinarrow};
            }
        } else if (!isOnTrailingHandler(parentSplitter) and
                   parentSplitter->indexOf(parentDecorator) > 0) {

            wdgt = parentSplitter->widget(parentSplitter->indexOf(parentDecorator) - 1);
            //do not join if the target widget is also splitted
            if (!wdgt->inherits("SplitterDecorator")){
                joinarrow=parentSplitter->orientation()==Qt::Horizontal?Qt::LeftArrow:Qt::UpArrow;
                externalOverlay = new Overlay{wdgt,joinarrow};
            }
        };

        //now show overlay if we started a valid joindrag
        if (externalOverlay){
            externalOverlay->show();
            internalOverlay=new Overlay{parentDecorator,Overlay::invertArrow(joinarrow)};
            internalOverlay->hide();
            dragaction=joinDrag;
            //qDebug() << "starting joindrag " << wdgt << " ( <-wdgt   parentDecorator->) " << parentDecorator;
        }
    }
}
int ExpanderCorner::pickCoordinate(int x,int y,Qt::Orientation orient)
{
    return orient==Qt::Horizontal?x:y;
}

int ExpanderCorner::pickSize(const QSize &size, Qt::Orientation orient)
{
    return (orient == Qt::Horizontal) ? size.width() : size.height();
}

void ExpanderCorner::enterEvent(QEvent *event)
{
    setCursor(Qt::CrossCursor); //after innersplit the cursor is still wrong when entering...
    Expander::enterEvent(event);
}

void ExpanderCorner::followDragJoiners(WidgetDecorator *parentDecorator, BlendSplitter *parentSplitter, int x, int y, Qt::Orientation /*splitorientation*/)
{
    x=pos().x()+x;
    y=pos().y()+y;

    if (isInContinuationOfSplitter(parentSplitter,x,y)){
        // maybe we need to change direction of the join ?
        Qt::Orientation o=parentSplitter->orientation();
        if ((isOnTrailingHandler(parentSplitter) and pickCoordinate(x,y,o)>pickSize(parentDecorator->size(),o))
                or (!isOnTrailingHandler(parentSplitter) and pickCoordinate(x,y,o)<0)
                ) {
            externalOverlay->show();
            internalOverlay->hide();
            setCursor(parentSplitter->orientation()==Qt::Horizontal?Qt::SizeHorCursor:Qt::SizeVerCursor);
        } else if ((isOnTrailingHandler(parentSplitter) and pickCoordinate(x,y,o)<=pickSize(parentDecorator->size(),o))
                   or (!isOnTrailingHandler(parentSplitter) and pickCoordinate(x,y,o)>=0)
                   ) {
            externalOverlay->hide();
            internalOverlay->show();
            setCursor(parentSplitter->orientation()==Qt::Horizontal?Qt::SizeHorCursor:Qt::SizeVerCursor);
        }
    } else {
        // hide all overlay since we dragged 'to the side' of the splitter
        externalOverlay->hide();
        internalOverlay->hide();
        setCursor(Qt::ForbiddenCursor);
    }
}



bool ExpanderCorner::isInContinuationOfSplitter(BlendSplitter *parentSplitter, int x, int y)
{
    if (parentSplitter->orientation() == Qt::Horizontal
        and y > 0 and y < parentSplitter->height()
        ) {
            return true;
    };
    if (parentSplitter->orientation() == Qt::Vertical
        and x > 0 and x < parentSplitter->width()
        ) {
            return true;
    };
    return false;
}

/**
 * @brief ExpanderCorner::decideDragAction
 * @param event
 *
 *If a drag is just started this function helps determine if we drag inwards
 *our outward and if it is a horizontal or vertical drag.
 */
void ExpanderCorner::decideDragAction(QMouseEvent *event, WidgetDecorator* parentDecorator, BlendSplitter* parentSplitter)
{
    int x=event->x()-hotspotX;
    int y=event->y()-hotspotY;

    if (abs(x) < BlendSplitter::expanderSize and abs(y) < BlendSplitter::expanderSize ){
        return;
    };

    // we dragged far enough
    dragorientation= (abs(x) > abs(y)) ? Qt::Horizontal : Qt::Vertical ;

    // but did we drag inwards?
    // (and als prefent splitting if widget already too small!)
    if ( x*unitX >0 and y*unitY>0 and (
             (dragorientation==Qt::Horizontal and parentDecorator->width()>2*BlendSplitter::expanderSize) or
             (dragorientation==Qt::Vertical and parentDecorator->height()>2*BlendSplitter::expanderSize))
         )
        {
            dragaction=splitDrag; // we dragged inwards
            performInnerSplit(parentDecorator,parentSplitter,dragorientation);
    }
    // do we join widgets together?
    else if (dragorientation == parentSplitter->orientation()){
        setupJoiners(parentDecorator,parentSplitter,x,y,dragorientation);
    } else {
        //here we could start a relocationdrag....
    }
}

void ExpanderCorner::mouseMoveEvent(QMouseEvent *event)
{
    if( !(event->buttons() & Qt::LeftButton)){
        return;
    }

    //get our parentdecorator and our immediate blendSplitter
    WidgetDecorator* parentDecorator{qobject_cast<WidgetDecorator*>(parentWidget())};
    if(parentDecorator == 0)
    {
        qCritical("A BlendSplitter library error occurred. Error code: 4");
        return;
    }
    BlendSplitter* parentSplitter{qobject_cast<BlendSplitter*>(parentDecorator->parentWidget())};
    if(parentSplitter == 0)
    {
        qCritical("A BlendSplitter library error occurred. Error code: 5");
        return;
    }

    switch (dragaction) {
    case undecidedDrag:
        decideDragAction(event,parentDecorator,parentSplitter);
        break;
    case joinDrag:
    {
        int x=event->x()-hotspotX;
        int y=event->y()-hotspotY;
        followDragJoiners(parentDecorator,parentSplitter,x,y,(abs(x) > abs(y)) ? Qt::Horizontal : Qt::Vertical);
    };
        break;
    case splitDrag:
    default:
        break;
    }
}

void ExpanderCorner::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        dragaction=undecidedDrag; //start of drag
        event->accept();    // No-op
    }
    else
    {
        releaseMouse();
        event->ignore();    // Propagate event
    }
}

void ExpanderCorner::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        //releasing leftbutton set us back to default cursor for handler
        // since we might have changed due to split or join
        setCursor(Qt::CrossCursor);
    };

    if(event->button() != Qt::LeftButton or dragaction != joinDrag){
        return;
    };

    //correct button and we have an overlay, so continue...
    if(externalOverlay->isVisible() && internalOverlay->isVisible())
    {
        qCritical("A BlendSplitter library error occurred. Error code: 8");
        return;
    }


    QWidget* widgetToRemove=nullptr; //improved readability later on
    QWidget* widgetToKeep=nullptr; //improved readability later on

    if (externalOverlay->isVisible() && !internalOverlay->isVisible()){
        widgetToKeep=internalOverlay->parentWidget();
        widgetToRemove=externalOverlay->parentWidget();
    } else if (!externalOverlay->isVisible() && internalOverlay->isVisible()){
        widgetToKeep=externalOverlay->parentWidget();
        widgetToRemove=internalOverlay->parentWidget();
    };


    // The visible overlays will be deleted when we remove the widget,
    // so we need to clean up the invisble overlays
    if (!internalOverlay->isVisible()){
        internalOverlay->deleteLater();
    };
    if (!externalOverlay->isVisible()){
        externalOverlay->deleteLater();
    };

    //do nothing if both overlays were hidden
    if (widgetToRemove==nullptr){
        externalOverlay = nullptr;
        internalOverlay = nullptr;
        return;
    }

    //first get our decorator and the parent blendSplitter
    WidgetDecorator* toKeepDecorator{qobject_cast<WidgetDecorator*>(widgetToKeep)};
    if(toKeepDecorator == 0)
    {
        qCritical("A BlendSplitter library error occurred. Error code: 1");
        return;
    }
    BlendSplitter* parentSplitter{qobject_cast<BlendSplitter*>(externalOverlay->parentWidget()->parentWidget())};
    if(parentSplitter == 0)
    {
        qCritical("A BlendSplitter library error occurred. Error code: 2");
        return;
    }


    //now delete the item with the visible overlay from the splitter

    QList<int> sizes{parentSplitter->sizes()};
    int toKeepIndex{parentSplitter->indexOf(toKeepDecorator)};
    int toRemoveIndex{parentSplitter->indexOf(widgetToRemove)};
    sizes[toKeepIndex] += sizes[toRemoveIndex] + 1;
    sizes.removeAt(toRemoveIndex);
    delete parentSplitter->widget(toRemoveIndex);
    externalOverlay = nullptr;
    internalOverlay = nullptr;

    // if we now have a blendSplitter with a single item, which is inside
    // another blendSplitter then we remove this singular-item splitter
    if(parentSplitter->count() == 1 and
    parentSplitter->parentWidget()->inherits("SplitterDecorator"))
    {
        BlendSplitter* newParent{qobject_cast<BlendSplitter*>(parentSplitter->parentWidget()->parentWidget())};
        if(newParent == nullptr)
        {
            qCritical("A BlendSplitter library error occurred. Error code: 3");
            return;
        }
        QList<int> sizes2{newParent->sizes()};
        newParent->insertDecoratedWidget(newParent->indexOf(parentSplitter->parentWidget()), toKeepDecorator);
        delete parentSplitter->parentWidget();
        newParent->setSizes(sizes2);
    }
    else
    {
        parentSplitter->setSizes(sizes);
    }

}
