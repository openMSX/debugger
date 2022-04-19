#include "BlendSplitter.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SplitterDecorator.h"
#include "SplitterHandle.h"
#include "WidgetDecorator.h"

int BlendSplitter::expanderSize{12};
int BlendSplitter::switchingBarHeight{36};
QString BlendSplitter::expanderImage{":/BlendSplitter/Expander"};

BlendSplitter::BlendSplitter(QWidget* (*defaultWidget) (), Qt::Orientation orientation) : QSplitter{orientation, nullptr}, defaultWidget{defaultWidget}
{
//    Q_INIT_RESOURCE(BlendSplitterResources);
    setChildrenCollapsible(false);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: black;}");
}

void BlendSplitter::addWidget()
{
    addWidget((*defaultWidget) ());
}

void BlendSplitter::addWidget(QWidget* widget)
{
    insertWidget(-1, widget);
}

void BlendSplitter::addWidget(RegistryItem* item)
{
    insertWidget(-1, item);
}

void BlendSplitter::insertWidget(int index)
{
    insertWidget(index, (*defaultWidget) ());
}

void BlendSplitter::insertWidget(int index, QWidget* widget)
{
    WidgetDecorator* decorator{new WidgetDecorator{widget}};
    QSplitter::insertWidget(index, decorator);
}

void BlendSplitter::insertWidget(int index, RegistryItem* item)
{
    insertWidget(index, new SwitchingWidget{item});
}

void BlendSplitter::addDecoratedWidget(WidgetDecorator* widget)
{
    insertDecoratedWidget(-1, widget);
}

void BlendSplitter::insertDecoratedWidget(int index, WidgetDecorator* widget)
{
    QSplitter::insertWidget(index, widget);
}

void BlendSplitter::addSplitter(BlendSplitter *splitter)
{
    insertSplitter(-1, splitter);
}

void BlendSplitter::insertSplitter(int index, BlendSplitter* splitter)
{
    SplitterDecorator* decorator{new SplitterDecorator{splitter}};
    QSplitter::insertWidget(index, decorator);
}

QSplitterHandle* BlendSplitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}
