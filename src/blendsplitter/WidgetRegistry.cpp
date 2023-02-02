#include "WidgetRegistry.h"

#include "RegistryItem.h"

WidgetRegistry::WidgetRegistry()
    : defaultItem{nullptr}
{
}

WidgetRegistry& WidgetRegistry::instance()
{
    static WidgetRegistry oneInstance;
    return oneInstance;
}

RegistryItem* WidgetRegistry::item(int i) const
{
    return list.value(i);
}

int WidgetRegistry::indexOf(RegistryItem* item) const
{
    return list.indexOf(item);
}

RegistryItem* WidgetRegistry::getDefault()
{
    if (defaultItem == nullptr) {
        if (list.size() == 0) {
            addItem(new RegistryItem{"Default",
                                     []()->QWidget* { return new QLabel{"Default widget"}; }});
        }
        defaultItem = item(0);
    }
    return defaultItem;
}

void WidgetRegistry::setDefault(RegistryItem* item)
{
    if (!list.contains(item)) {
        addItem(item);
    }
    defaultItem = item;
}

void WidgetRegistry::setDefault(int index)
{
    if (index < size()) {
        setDefault(item(index));
    }
}

void WidgetRegistry::addItem(RegistryItem* item)
{
    list.append(item);
    emit registryChanged();
}

void WidgetRegistry::insertItem(int index, RegistryItem* item)
{
    list.insert(index, item);
    emit registryChanged();
}

void WidgetRegistry::removeItem(RegistryItem* item)
{
    removeItem(indexOf(item));
}

void WidgetRegistry::removeItem(int index)
{
    list.removeAt(index);
    emit registryChanged();
}

int WidgetRegistry::size() const
{
    return list.size();
}
