#include "TabRenamerHelper.h"
#include <QTabBar>
#include <QLineEdit>
#include <QTabWidget>
#include <QMouseEvent>
#include <QKeyEvent>

TabRenamerHelper::TabRenamerHelper(QTabWidget *parent)
    : QObject{parent}, workspaces{parent},
      tabLineEdit{nullptr}, editedTab{-1}
{
    connect(workspaces->tabBar(), &QTabBar::tabBarDoubleClicked,
            this, &TabRenamerHelper::tabBarDoubleClicked);
    workspaces->tabBar()->installEventFilter(this);
//    workspaces->installEventFilter(this);

}

void TabRenamerHelper::tabBarDoubleClicked(int index)
{
    editedTab = index;
    if (index == -1) {
        return;
    }
    QRect r = workspaces->tabBar()->tabRect(index);
    QString t = workspaces->tabBar()->tabText(index);
    tabLineEdit = new QLineEdit{workspaces->tabBar()};
    tabLineEdit->show();
    tabLineEdit->move(r.topLeft());
    tabLineEdit->resize(r.size());
    tabLineEdit->setText(t);
    tabLineEdit->selectAll();
    tabLineEdit->setFocus();
    tabLineEdit->installEventFilter(this);
    connect(tabLineEdit, &QLineEdit::editingFinished, this, &TabRenamerHelper::tabNameEditingFinished);
}


void TabRenamerHelper::tabNameEditingFinished()
{
    QString newname = tabLineEdit->text();
    if (!newname.isEmpty()) {
        workspaces->tabBar()->setTabText(editedTab, newname);
    }
    tabLineEdit->deleteLater();
    tabLineEdit = nullptr;
}

bool TabRenamerHelper::eventFilter(QObject* obj, QEvent* event)
{
    bool result = QObject::eventFilter(obj, event);
    QTabBar* bar = workspaces->tabBar();
    if (obj == bar) {
        if (event->type() == QEvent::MouseButtonPress) {
            // the user is trying to move away from editing by clicking another tab
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            int clickedTabId = bar->tabAt(me->pos());
            if (editedTab != -1 && editedTab != clickedTabId && tabLineEdit) {
                emit tabLineEdit->editingFinished();
                return false;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            // Perhaps we need to start a new name editing action?
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            int clickedTabId = bar->tabAt(me->pos());
            if (clickedTabId == -1) {
                return result;
            }
            if (editedTab != -1 && editedTab != clickedTabId && tabLineEdit) {
                emit tabLineEdit->editingFinished();
                return false;
            }
        }
    }
    //handle some events on the line edit to make it behave itself nicely as a rename editor
    if (obj == tabLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            disconnect(tabLineEdit, &QLineEdit::editingFinished, this, &TabRenamerHelper::tabNameEditingFinished);
            tabLineEdit->deleteLater();
            tabLineEdit = nullptr;
            return true; // no further handling of this event is required
        }
    }

    return result;
}
