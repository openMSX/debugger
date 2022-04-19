#include "SwitchingWidget.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SwitchingBar.h"
#include "SwitchingCombo.h"

SwitchingWidget::SwitchingWidget(RegistryItem* item, QWidget* parent,bool menuAtTop) : QSplitter(Qt::Vertical, parent), bar{new SwitchingBar{}}, widgetEnabled(true),barAtTop(menuAtTop)
{
    setChildrenCollapsible(true);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: grey;}");
    if (barAtTop){
        addWidget(bar);
        addWidget((*WidgetRegistry::getRegistry()->getDefault()->widget) ());
    } else {
        addWidget((*WidgetRegistry::getRegistry()->getDefault()->widget) ());
        addWidget(bar);

    }
    bar->reconstruct(*WidgetRegistry::getRegistry()->getDefault()->populateBar, widget(widgetIndex()));
    connect(bar->combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SwitchingWidget::changeCurrentWidget);
    setCurrentWidget(item);
}

void SwitchingWidget::setCurrentWidget(RegistryItem *item)
{
    if(item == nullptr)
    {
        item = WidgetRegistry::getRegistry()->getDefault();
    }
    if(WidgetRegistry::getRegistry()->indexOf(item) >= 0)
    {
        delete widget(widgetIndex());
        insertWidget(widgetIndex(), (*item->widget) ());
        bar->reconstruct(*item->populateBar, widget(widgetIndex()));
        bar->combo->setCurrentIndex(bar->combo->findText(item->name));
        widget(widgetIndex())->setEnabled(widgetEnabled);
    }
}

bool SwitchingWidget::getEnableWidget()
{
    return widgetEnabled;
}

void SwitchingWidget::setCurrentIndex(int index)
{
    setCurrentWidget(WidgetRegistry::getRegistry()->item(index));
}

int SwitchingWidget::getCurrentIndex()
{
    return bar->combo->currentIndex();
}

void SwitchingWidget::setEnableWidget(bool enable)
{
    widgetEnabled=enable;

    QWidget* wdgt=widget(widgetIndex());
    if (wdgt != nullptr){
        wdgt->setEnabled(enable);
    }
}

void SwitchingWidget::changeCurrentWidget(int index)
{
    if(index >= 0)
    {
        setCurrentWidget(WidgetRegistry::getRegistry()->item(index));
    }
}

int SwitchingWidget::barIndex()
{
    return barAtTop?0:1;
}

int SwitchingWidget::widgetIndex()
{
    return barAtTop?1:0;
}
