// $Id$

#include "DockableWidgetArea.h"
#include "DockableWidgetLayout.h"
#include "DockableWidget.h"
#include <QPainter>

DockableWidgetArea::DockableWidgetArea( QWidget* parent )
	: QWidget( parent )
{
	layout = new DockableWidgetLayout;
	setLayout(layout);
}

void DockableWidgetArea::removeWidget( DockableWidget* widget )
{
	layout->removeWidget( widget );
	widget->setParent(0);
}

void DockableWidgetArea::addWidget( DockableWidget *widget, const QRect& rect )
{
	widget->setParent(this);
	QRect r( rect );
	r.moveTopLeft( mapFromGlobal( r.topLeft() ) );
	layout->addWidget( widget, r );
}

void DockableWidgetArea::addWidget( DockableWidget *widget, DockableWidgetLayout::DockSide side, int distance, int w, int h )
{
	widget->setParent(this);
	layout->addWidget( widget, side, distance, w, h );
}

void DockableWidgetArea::paintEvent( QPaintEvent * e )
{
	QWidget::paintEvent(e);
}

bool DockableWidgetArea::insertLocation( QRect& r, const QSizePolicy& sizePol )
{
	r.moveTopLeft( mapFromGlobal( r.topLeft() ) );
	bool ok = layout->insertLocation( r, sizePol );
	r.moveTopLeft( mapToGlobal( r.topLeft() ) );
	return ok;
}
