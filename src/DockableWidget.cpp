// $Id$

#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionDockWidget>
#include <QRubberBand>
#include <QMouseEvent>
#include "DockableWidget.h"
#include "DockManager.h"


DockableWidget::DockableWidget( DockManager& manager, QWidget* parent)
	: QWidget( parent ), dockManager( manager )
{
	mainWidget = 0;
	floating = false;
	movable = true;
	closable = true;
	destroyable = true;
	dragging = false;
	setAttribute( Qt::WA_DeleteOnClose, true );
	
	titleLabel = new QLabel;
	closeButton = new QToolButton;
	closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	closeButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
	int sz = style()->pixelMetric(QStyle::PM_SmallIconSize);
	closeButton->setIconSize( style()->standardIcon(QStyle::SP_TitleBarCloseButton).actualSize(QSize(sz, sz)) );
	closeButton->setAutoRaise(true);
	connect( closeButton, SIGNAL( clicked() ), this, SLOT( close() ) );
	
	headerLayout = new QHBoxLayout;
	headerLayout->setMargin(0);
	headerLayout->addWidget( titleLabel, 1 );
	headerLayout->addWidget( closeButton, 0 );
	
	headerWidget = new QWidget;
	headerWidget->setLayout( headerLayout );

	widgetLayout = new QVBoxLayout;
	widgetLayout->setMargin(3);
	widgetLayout->setSpacing(1);
	widgetLayout->addWidget( headerWidget );
	setLayout( widgetLayout );
	
	rubberBand = new QRubberBand( QRubberBand::Rectangle );
}

DockableWidget::~DockableWidget()
{
	if(mainWidget) delete mainWidget;
	delete rubberBand;
}

QWidget *DockableWidget::widget() const
{
	return mainWidget;
}

void DockableWidget::setWidget(QWidget *widget)
{
	if( mainWidget ) {
		widgetLayout->removeWidget( mainWidget );
	}
	
	mainWidget = widget;
	
	if( widget ) {
		widgetLayout->addWidget( widget, 1 );
		setSizePolicy( widget->sizePolicy() );
	}
}

const QString& DockableWidget::id() const
{
	return widgetId;
}

void DockableWidget::setId( const QString& str )
{
	widgetId = str;
}

QString DockableWidget::title() const
{
	return windowTitle();
}

void DockableWidget::setTitle(const QString& title)
{
	setWindowTitle( title );
	titleLabel->setText( title + ':' );
}

bool DockableWidget::isFloating() const
{
	return floating;
}

void DockableWidget::setFloating(bool enable, bool showNow)
{
	if( floating != enable ) {
		floating = enable;
	
		setWindowFlags( floating ? Qt::Tool : Qt::Widget );
		if( floating && showNow ) {
			show();
		}
	}
}

bool DockableWidget::isMovable() const
{
	return movable;
}

void DockableWidget::setMovable(bool enable)
{
	if( movable != enable ) {
		movable = enable;
	}
}

bool DockableWidget::isClosable() const
{
	return closable;
}

void DockableWidget::setClosable(bool enable)
{
	if( closable != enable ) {
		closable = enable;
		if( enable ) {
			closeButton->show();
		} else {
			closeButton->hide();
		}
		dragging = false;
		if( rubberBand ) {
			rubberBand->hide();
		}
	}
}

bool DockableWidget::isDestroyable() const
{
	return destroyable;
}

void DockableWidget::setDestroyable(bool enable)
{
	destroyable = enable;
	setAttribute( Qt::WA_DeleteOnClose, enable );
}

void DockableWidget::closeEvent( QCloseEvent * event )
{
	if( closable || destroyable ) {
		if( destroyable && floating ) {
			dockManager.undockWidget(this);
			event->accept();
		} else {
			hide();
			event->ignore();
			emit visibilityChanged( this );
		}
	} else
		event->ignore();
}

void DockableWidget::mousePressEvent ( QMouseEvent * event )
{
	if( movable && event->button() == Qt::LeftButton ) {
		dragging = true;
		dragStart = event->globalPos();
		dragOffset = event->pos();
	}
}

void DockableWidget::mouseMoveEvent ( QMouseEvent * event )
{
	if( dragging ) {
		if( event->buttons() & Qt::LeftButton ) {
			// dragging of widget in progress, update rubberband
			if( !rubberBand->isVisible() ) {
				if( abs( event->globalX()-dragStart.x() ) > 10 ||
					 abs( event->globalY()-dragStart.y() ) > 10 ) {
					rubberBand->resize( width(), height() );
					rubberBand->move( event->globalX()-dragOffset.x(), event->globalY()-dragOffset.y() );
					rubberBand->show();
				}
			} else {
				QRect r( event->globalX()-dragOffset.x(), event->globalY()-dragOffset.y(), width(), height() );
				if( floating && dockManager.insertLocation(r, sizePolicy()) ) {
					rubberBand->move( r.x(), r.y() );
					rubberBand->resize( r.width(), r.height() );
				} else {
					rubberBand->move( event->globalX()-dragOffset.x(), event->globalY()-dragOffset.y() );
					rubberBand->resize( width(), height() );
				}
			}
		} else {
			dragging = false;
			if( rubberBand ) {
				rubberBand->hide();
			}
		}
	}
}

void DockableWidget::mouseReleaseEvent ( QMouseEvent * event )
{
	if( dragging ) {
		// dragging
		dragging = false;
		if( rubberBand ) {
			rubberBand->hide();
		}
		// only do anything if this was a meaningful drag
		if( abs( event->globalX()-dragStart.x() ) > 10 ||
		    abs( event->globalY()-dragStart.y() ) > 10 ) {
			if( movable ) {
				if( floating ) {
					QRect mouseRect( event->globalX() - dragOffset.x(), event->globalY() - dragOffset.y(), width(), height() );
					QRect r(mouseRect);
					if( dockManager.insertLocation( r, sizePolicy() ) ) {
						setFloating( false );
						dockManager.dockWidget( this, QPoint(), mouseRect );
						
					} else {
						move( event->globalPos() - dragOffset );
					}
				} else {
					dockManager.undockWidget(this);
					setFloating(true);
					move( event->globalPos() - dragOffset );
				}
			}
		}
	}
}
