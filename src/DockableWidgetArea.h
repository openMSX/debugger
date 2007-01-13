// $Id$

#ifndef _DOCKABLETWIDGETAREA_H
#define _DOCKABLETWIDGETAREA_H

#include <QWidget>
#include "DockableWidgetLayout.h"

class DockableWidget;
class QPaintEvent;

class DockableWidgetArea : public QWidget
{
	Q_OBJECT;
public:
	DockableWidgetArea( QWidget* parent = 0 );
	//~DockableWidgetArea();

	friend class DockManager;

protected:
	void paintEvent( QPaintEvent * e );

private:
	DockableWidgetLayout *layout;

	void removeWidget( DockableWidget* widget );
	void addWidget( DockableWidget *widget, const QRect& rect );
	void addWidget( DockableWidget *widget, DockableWidgetLayout::DockSide side,
	                int distance, int width = -1, int height = -1 );
	bool insertLocation( QRect& r, const QSizePolicy& sizePol );
};

#endif    // _DOCKABLETWIDGETAREA_H
