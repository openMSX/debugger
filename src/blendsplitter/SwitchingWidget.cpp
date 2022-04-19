#include "SwitchingWidget.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SwitchingBar.h"
#include "SwitchingCombo.h"

SwitchingWidget::SwitchingWidget(RegistryItem* item, QWidget* parent) : QSplitter(Qt::Vertical, parent), bar{new SwitchingBar{}}
{
    setChildrenCollapsible(true);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: grey;}");
    addWidget((*WidgetRegistry::getRegistry()->getDefault()->widget) ());
    addWidget(bar);
    bar->reconstruct(*WidgetRegistry::getRegistry()->getDefault()->populateBar, widget(0));
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
        delete widget(0);
        insertWidget(0, (*item->widget) ());
        bar->reconstruct(*item->populateBar, widget(0));
        bar->combo->setCurrentIndex(bar->combo->findText(item->name));
    }
}

void SwitchingWidget::changeCurrentWidget(int index)
{
    if(index >= 0)
    {
        setCurrentWidget(WidgetRegistry::getRegistry()->item(index));
    }
}
