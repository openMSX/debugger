#include "SwitchingCombo.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"

SwitchingCombo::SwitchingCombo()
{
    connect(WidgetRegistry::getRegistry(), &WidgetRegistry::registryChanged, this, &SwitchingCombo::repopulate);
    repopulate();
}

void SwitchingCombo::repopulate()
{
    RegistryItem* current{WidgetRegistry::getRegistry()->item(currentIndex())};
    clear();
    for(int i{0}; i < WidgetRegistry::getRegistry()->size(); i++)
    {
        QComboBox::addItem(WidgetRegistry::getRegistry()->item(i)->name);
    }
    if(current != 0)
    {
        setCurrentIndex(findText(current->name));
    }
    else
    {
        setCurrentIndex(findText(WidgetRegistry::getRegistry()->getDefault()->name));
    }
}
