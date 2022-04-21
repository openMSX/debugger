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
    if (popupmenu){
        delete popupmenu; //Qmenu has ownership of the actions
        popupmenu=nullptr;
    };
    /*
    delete popupmenu;
    This sometimes causes a crash????
    Default CPU workspace and grep handler to close "CPU registers"/"Flags"
    wiggled a bit left/right when joining and sometimes this happened????
    1  SplitterHandle::~SplitterHandle                  SplitterHandle.cpp         15   0x5555556b6d3e
    2  SplitterHandle::~SplitterHandle                  SplitterHandle.cpp         16   0x5555556b6d90
    3  QSplitterLayoutStruct::~QSplitterLayoutStruct    qsplitter_p.h              74   0x7ffff7a65bfd
    4  QSplitter::childEvent                            qsplitter.cpp              1312 0x7ffff7a65bfd
    5  QObject::event                                   qobject.cpp                1361 0x7ffff658abcb
    6  QWidget::event                                   qwidget.cpp                9094 0x7ffff79099d3
    7  QFrame::event                                    qframe.cpp                 550  0x7ffff79b179e
    8  QApplicationPrivate::notify_helper               qapplication.cpp           3685 0x7ffff78cab0c
    9  QApplication::notify                             qapplication.cpp           3431 0x7ffff78d1a90
    10 QCoreApplication::notifyInternal2                qcoreapplication.cpp       1075 0x7ffff655bc48
    11 QCoreApplication::sendEvent                      qcoreapplication.cpp       1470 0x7ffff655bdfe
    12 QObjectPrivate::setParent_helper                 qobject.cpp                2166 0x7ffff6590082
    13 QObject::~QObject                                qobject.cpp                1118 0x7ffff6590592
    14 QWidget::~QWidget                                qwidget.cpp                1408 0x7ffff790526c
    15 WidgetDecorator::~WidgetDecorator                WidgetDecorator.cpp        26   0x5555556b8852
    16 WidgetDecorator::~WidgetDecorator                WidgetDecorator.cpp        29   0x5555556b887c
    17 ExpanderCorner::mouseReleaseEvent                ExpanderCorner.cpp         409  0x5555556b5239
    18 QWidget::event                                   qwidget.cpp                9033 0x7ffff7909670
    19 QFrame::event                                    qframe.cpp                 550  0x7ffff79b179e
    20 QApplicationPrivate::notify_helper               qapplication.cpp           3685 0x7ffff78cab0c
    21 QApplication::notify                             qapplication.cpp           3129 0x7ffff78d26f8
    22 QCoreApplication::notifyInternal2                qcoreapplication.cpp       1075 0x7ffff655bc48
    23 QCoreApplication::sendSpontaneousEvent           qcoreapplication.cpp       1482 0x7ffff655be0e
    24 QApplicationPrivate::sendMouseEvent              qapplication.cpp           2615 0x7ffff78d0fda
    25 QWidgetWindow::handleMouseEvent                  qwidgetwindow.cpp          674  0x7ffff7922dd1
    26 QWidgetWindow::event                             qwidgetwindow.cpp          295  0x7ffff7925a1b
    27 QApplicationPrivate::notify_helper               qapplication.cpp           3685 0x7ffff78cab0c
    28 QApplication::notify                             qapplication.cpp           3431 0x7ffff78d1a90
    29 QCoreApplication::notifyInternal2                qcoreapplication.cpp       1075 0x7ffff655bc48
    30 QCoreApplication::sendSpontaneousEvent           qcoreapplication.cpp       1482 0x7ffff655be0e
    31 QGuiApplicationPrivate::processMouseEvent        qguiapplication.cpp        2203 0x7ffff6fb9738
    32 QGuiApplicationPrivate::processWindowSystemEvent qguiapplication.cpp        1935 0x7ffff6fbac15
    33 QWindowSystemInterface::sendWindowSystemEvents   qwindowsysteminterface.cpp 1170 0x7ffff6f96f4b
    34 xcbSourceDispatch                                qxcbeventdispatcher.cpp    105  0x7ffff2a0a33a
    35 g_main_context_dispatch                                                          0x7ffff392817d
    36 ??                                                                               0x7ffff3928400
    37 g_main_context_iteration                                                         0x7ffff39284a3
    38 QEventDispatcherGlib::processEvents              qeventdispatcher_glib.cpp  423  0x7ffff65b473c
    39 QEventLoop::exec                                 qeventloop.cpp             225  0x7ffff655a662
    40 QCoreApplication::exec                           qcoreapplication.cpp       1383 0x7ffff6563590
    41 main                                             main.cpp                   20   0x5555556b9e54
    */
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
