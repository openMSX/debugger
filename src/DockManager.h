// $Id:$

#ifndef _DOCKMANAGER_H
#define _DOCKMANAGER_H

#include "DockableWidgetLayout.h"

#include <QList>
#include <QMap>

class QWidget;
class QRect;
class QPoint;
class DockableWidget;
class DockableWidgetArea;

class DockManager 
{
public:
	DockManager();
	~DockManager();
	
	void addDockArea( DockableWidgetArea *area );
	int dockAreaIndex( DockableWidgetArea *area );
	
	void insertWidget( DockableWidget *widget, int index,
	                   DockableWidgetLayout::DockSide side, int distance, int w = -1, int h = -1 ); 
	void dockWidget( DockableWidget *widget, const QPoint& p, const QRect& r );
	void undockWidget( DockableWidget *widget );

	bool insertLocation( QRect& r, const QSizePolicy& sizePol );
	
private:
	QList<DockableWidgetArea*> areas;
	QMap<DockableWidget*, DockableWidgetArea*> areaMap;
};

#endif    // _DOCKMANAGER_H
