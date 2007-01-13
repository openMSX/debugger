// $Id$

#include "DockManager.h"
#include "DockableWidget.h"
#include "DockableWidgetArea.h"

DockManager::DockManager()
{
}

DockManager::~DockManager()
{
}

void DockManager::addDockArea( DockableWidgetArea *area )
{
	if( areas.indexOf( area ) == -1 )
		areas.append( area );
}

int DockManager::dockAreaIndex( DockableWidgetArea *area )
{
	return areas.indexOf( area );
}

void DockManager::dockWidget( DockableWidget *widget, const QPoint& p, const QRect& r )
{
	QMap<DockableWidget*, DockableWidgetArea*>::iterator it = areaMap.begin(); // TODO
	if( it != areaMap.end() ) {
		return it.value()->addWidget( widget, r );
	}
}

void DockManager::undockWidget( DockableWidget *widget )
{
	QMap<DockableWidget*, DockableWidgetArea*>::iterator it = areaMap.find( widget );
	if( it != areaMap.end() ) {
		it.value()->removeWidget( widget );
	}
}

void DockManager::insertWidget( DockableWidget *widget, int index,
	                   DockableWidgetLayout::DockSide side, int distance, int w , int h ) 
{
	if( index<0 || index>=areas.size() ) return;

	Q_ASSERT( areaMap.find(widget) == areaMap.end() );
	
	areas[index]->addWidget( widget, side, distance, w, h );
	areaMap[widget] = areas[index];
}

bool DockManager::insertLocation( QRect& r, const QSizePolicy& sizePol )
{
	QMap<DockableWidget*, DockableWidgetArea*>::iterator it = areaMap.begin(); // TODO
	if( it != areaMap.end() ) {
		return it.value()->insertLocation( r, sizePol );
	}
	return false;
}
