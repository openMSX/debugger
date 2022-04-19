#include "SwitchingBar.h"

#include <QMenuBar>
#include "BlendSplitter.h"
#include "SwitchingCombo.h"

void SwitchingBar::addMenu(QMenu* menu)
{
    QMenuBar* menuBar{new QMenuBar};
    menuBar->setDefaultUp(true);
    layout->insertWidget(layout->count() - 1, menuBar);
    layout->setAlignment(menuBar, Qt::AlignVCenter);
    setStyleSheet("QMenuBar{background-color: transparent;}");
    menuBar->addMenu(menu);
}

void SwitchingBar::addWidget(QWidget* widget)
{
    layout->insertWidget(layout->count() - 1, widget);
}

SwitchingBar::SwitchingBar(QWidget* parent) : QWidget(parent), layout{new QHBoxLayout{}}, combo{new SwitchingCombo{}}
{
    layout->setContentsMargins(BlendSplitter::expanderSize * 3 / 4, 0, 0, 0);
    setLayout(layout);
    setMinimumHeight(BlendSplitter::switchingBarHeight);
    setMaximumHeight(BlendSplitter::switchingBarHeight);
    layout->addWidget(combo);
    layout->addStretch();
}

void SwitchingBar::reconstruct(void (*populateBar) (SwitchingBar*, QWidget*), QWidget* widget)
{
    int count{layout->count() - 1};
    for(int i = 1; i < count; i++)
    {
        QLayoutItem* it{layout->takeAt(1)};
        delete it->widget();
        delete it;
    }
    (*populateBar) (this, widget);
}

SwitchingBar::~SwitchingBar()
{
    delete layout;
}
