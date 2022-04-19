#include "SplitterHandle.h"
#include <QAction>
#include <QMenu>
#include <QDebug>

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QSplitter* parent) : QSplitterHandle(orientation, parent)
{
    popupmenu= new QMenu(this);
    splitAction= popupmenu->addAction(tr("Split"));
    joinAction= popupmenu->addAction(tr("Join"));
}

SplitterHandle::~SplitterHandle()
{
    delete popupmenu; //Qmenu has ownership of the actions
}

void SplitterHandle::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::RightButton){
        QPoint pos=event->globalPos();
        // When positioning a menu with exec() or popup(), bear in mind that you
        // cannot rely on the menu's current size(). For performance reasons,
        // the menu adapts its size only when necessary, so in many cases, the
        // size before and after the show is different. Instead, use sizeHint()
        // which calculates the proper size depending on the menu's current
        // contents.
        pos.setX(pos.x() - popupmenu->sizeHint().width() / 2); //
        QAction* act=popupmenu->exec(pos);
        if (act==joinAction){
            //not yet implemented
        } else if (act==splitAction){
            //not yet implemented
        }
//        setFocus();
//        grabMouse();
    }
    QSplitterHandle::mousePressEvent(event);
}
void SplitterHandle::mouseReleaseEvent(QMouseEvent* event)
{
    QSplitterHandle::mouseReleaseEvent(event);
    releaseMouse();
}

bool SplitterHandle::event(QEvent *event)
{
//    qDebug() << " SplitterHandle::event " << event ;
    return QSplitterHandle::event(event);
}
