#include "DockManager.h"
#include "DockableWidget.h"
#include "DockableWidgetArea.h"


void DockManager::addDockArea(DockableWidgetArea* area)
{
	if (areas.indexOf(area) == -1) {
		areas.append(area);
	}
}

int DockManager::dockAreaIndex(DockableWidgetArea* area) const
{
	return areas.indexOf(area);
}

void DockManager::dockWidget(DockableWidget* widget, const QPoint& p, const QRect& r)
{
	AreaMap::iterator it = areaMap.begin(); // TODO
	if (it != areaMap.end()) {
		areaMap[widget] = it.value();
		return it.value()->addWidget(widget, r);
	}
}

void DockManager::undockWidget(DockableWidget* widget)
{
	AreaMap::iterator it = areaMap.find(widget);
	if (it != areaMap.end()) {
		it.value()->removeWidget(widget);
	}
}

void DockManager::insertWidget(
		DockableWidget* widget, int index,
		DockableWidgetLayout::DockSide side, int distance, int w, int h)
{
	if (index < 0 || index >= areas.size()) return;

	//Q_ASSERT(areaMap.find(widget) == areaMap.end());

	areas[index]->addWidget(widget, side, distance, w, h);
	areaMap[widget] = areas[index];
}

bool DockManager::insertLocation(QRect& r, const QSizePolicy& sizePol)
{
	AreaMap::iterator it = areaMap.begin(); // TODO
	if (it == areaMap.end()) return false;

	return it.value()->insertLocation(r, sizePol);
}

void DockManager::visibilityChanged(DockableWidget* widget)
{
	AreaMap::iterator it = areaMap.find(widget);
	if (it != areaMap.end()) {
		it.value()->layout->changed();
	}
}

void DockManager::getConfig(int index, QStringList& list) const
{
	areas[index]->getConfig(list);
}

void DockManager::attachWidget(DockableWidget* widget)
{
	dockWidgets.append(widget);
}

void DockManager::detachWidget(DockableWidget* widget)
{
	dockWidgets.removeAll(widget);
}

const QList<DockableWidget*>& DockManager::managedWidgets() const
{
	return dockWidgets;
}

DockableWidget* DockManager::findDockableWidget(const QString& id) const
{
	for (QList<DockableWidget*>::const_iterator it = dockWidgets.begin();
	     it != dockWidgets.end(); ++it) {
		if ((*it)->id() == id) return *it;
	}
	return 0;
}
