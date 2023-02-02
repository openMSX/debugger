#include "SwitchingCombo.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"

SwitchingCombo::SwitchingCombo()
{
    connect(&WidgetRegistry::instance(), &WidgetRegistry::registryChanged, this, &SwitchingCombo::repopulate);
    repopulate();
}

void SwitchingCombo::repopulate()
{
    auto& registry = WidgetRegistry::instance();
    auto* current = registry.item(currentIndex());
    clear();
    for (int i = 0; i < registry.size(); ++i) {
        QComboBox::addItem(registry.item(i)->name);
    }
    if (current) {
        setCurrentIndex(findText(current->name));
    } else {
        setCurrentIndex(findText(registry.getDefault()->name));
    }
}
