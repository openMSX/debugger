#include "Expander.h"

#include "BlendSplitter.h"
#include "Overlay.h"
#include "WidgetDecorator.h"

Expander::Expander(WidgetDecorator* parent)
    : QLabel(parent)
    , pixmap{new QPixmap{BlendSplitter::expanderImage}}
    , overlay{nullptr}
{
    *pixmap = pixmap->scaledToHeight(BlendSplitter::expanderSize, Qt::FastTransformation);
    setPixmap(*pixmap);
    resize(BlendSplitter::expanderSize, BlendSplitter::expanderSize);
    setCursor(Qt::WhatsThisCursor);
}

void Expander::reposition()
{
    raise();
}

//void Expander::mousePressEvent(QMouseEvent* event)
//{
//    if (event->button() == Qt::LeftButton) {
//        event->accept();    // No-op
//    } else {
//        releaseMouse();
//        event->ignore();    // Propagate event
//    }
//}

//void Expander::mouseReleaseEvent(QMouseEvent* event)
//{
//    if (event->button() == Qt::LeftButton && overlay != nullptr) {
//        auto* parentDecorator = qobject_cast<WidgetDecorator*>(parentWidget());
//        if (!parentDecorator) {
//            qCritical("A BlendSplitter library error occurred. Error code: 1");
//            return;
//        }
//        auto* parentSplitter = qobject_cast<BlendSplitter*>(overlay->parentWidget()->parentWidget());
//        if (!parentSplitter) {
//            qCritical("A BlendSplitter library error occurred. Error code: 2");
//            return;
//        }
//        QList<int> sizes{parentSplitter->sizes()};
//        int parentIndex{parentSplitter->indexOf(parentDecorator)};
//        int overlayIndex{parentSplitter->indexOf(overlay->parentWidget())};
//        sizes[parentIndex] += sizes[overlayIndex] + 1;
//        sizes.removeAt(overlayIndex);
//        delete parentSplitter->widget(overlayIndex);
//        if (parentSplitter->count() == 1 && parentSplitter->parentWidget()->inherits("SplitterDecorator")) {
//            auto* newParent = qobject_cast<BlendSplitter*>(parentSplitter->parentWidget()->parentWidget());
//            if (!newParent) {
//                qCritical("A BlendSplitter library error occurred. Error code: 3");
//                return;
//            }
//            QList<int> sizes2{newParent->sizes()};
//            newParent->insertDecoratedWidget(newParent->indexOf(parentSplitter->parentWidget()), parentDecorator);
//            delete parentSplitter->parentWidget();
//            newParent->setSizes(sizes2);
//        } else {
//            parentSplitter->setSizes(sizes);
//        }
//        overlay = nullptr;
//    }
//}

Expander::~Expander()
{
    delete pixmap;
}

//void Expander::enterEvent(QEvent* event)
//{
//    setCursor(Qt::SizeAllCursor);
//}

//void Expander::leaveEvent(QEvent* event)
//{
//    setCursor(Qt::ArrowCursor);
//}
