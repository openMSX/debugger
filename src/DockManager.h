#ifndef DOCKMANAGER_H
#define DOCKMANAGER_H

#include "DockableWidgetLayout.h"
#include <QList>
#include <QMap>

class QRect;
class QPoint;
class DockableWidget;
class DockableWidgetArea;

class DockManager
{
public:
	void addDockArea(DockableWidgetArea* area);
	int dockAreaIndex(DockableWidgetArea* area) const;

	void insertWidget(DockableWidget* widget, int index,
	                  DockableWidgetLayout::DockSide side, int distance,
	                  int w = -1, int h = -1);
	void dockWidget(DockableWidget* widget, const QPoint& p, const QRect& r);
	void undockWidget(DockableWidget* widget);

	bool insertLocation(QRect& r, const QSizePolicy& sizePol);

	void visibilityChanged(DockableWidget* widget);
	void getConfig(int index, QStringList& list) const;

	void attachWidget(DockableWidget* widget);
	void detachWidget(DockableWidget* widget);
	const QList<DockableWidget*>& managedWidgets() const;
	DockableWidget* findDockableWidget(const QString& id) const;

private:
	typedef QMap<DockableWidget*, DockableWidgetArea*> AreaMap;
	AreaMap areaMap;
	QList<DockableWidgetArea*> areas;
	QList<DockableWidget*> dockWidgets;
};

#endif // DOCKMANAGER_H
