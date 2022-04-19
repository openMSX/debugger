#include "RegistryItem.h"

RegistryItem::RegistryItem(QString name, QWidget* (*widget) (), void (*populateBar) (SwitchingBar*, QWidget*)) : name{name}, widget{widget}, populateBar{populateBar} {}
