#include "RegistryItem.h"

RegistryItem::RegistryItem(const QString& name,
                           std::function<QWidget*()> widget,
                           std::function<void(SwitchingBar*, QWidget*)> populateBar)
    : name{name}
    , widget{widget}
    , populateBar{populateBar}
{
}
