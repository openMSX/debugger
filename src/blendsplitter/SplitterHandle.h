#ifndef SPLITTERHANDLE_H
#define SPLITTERHANDLE_H

#include "Global.h"

class BlendSplitter;
class QAction;
class QMenu;

class SplitterHandle final : public QSplitterHandle
{
    Q_OBJECT

public:
    SplitterHandle(const SplitterHandle&) = delete;
    SplitterHandle& operator=(const SplitterHandle&) = delete;
    SplitterHandle(Qt::Orientation orientation, QSplitter* parent);
    ~SplitterHandle();

protected:
    void createPopupMenu();
    void destroyPopupMenu();

    void removeFromSplitter(int toKeepIndex, int toRemoveIndex);

protected slots:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    QMenu* popupmenu;
    QAction* joinBeforeAction;
    QAction* joinAfterAction;
    QAction* splitHoriBeforeAction;
    QAction* splitHoriAfterAction;
    QAction* splitVertBeforeAction;
    QAction* splitVertAfterAction;
};

#endif
