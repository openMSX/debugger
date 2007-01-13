// $Id$

#ifndef _DOCKABLEWIDGET_H
#define _DOCKABLEWIDGET_H

#include <QWidget>
#include <QString>

class QLabel;
class QToolButton;
class QHBoxLayout;
class QVBoxLayout;
class QRubberBand;
class DockManager;

class DockableWidget : public QWidget
{
	Q_OBJECT;
public:
	DockableWidget( DockManager& manager, QWidget* parent = 0 );
	~DockableWidget();

	QWidget *widget() const;
	void setWidget(QWidget *widget);

	const QString title() const;
	void setTitle(const QString& title);

	const bool isFloating() const;
	void setFloating(bool enable);
	const bool isMovable() const;
	void setMovable(bool enable);
	const bool isClosable() const;
	void setClosable(bool enable);
	const bool isDestroyable() const;
	void setDestroyable(bool enable);

protected:
	//virtual void moveEvent ( QMoveEvent * event );
	virtual void mousePressEvent ( QMouseEvent * event );
	virtual void mouseMoveEvent ( QMouseEvent * event );
	virtual void mouseReleaseEvent ( QMouseEvent * event );

private:
	DockManager& dockManager;

	bool floating;
	bool closable, movable, destroyable;
	bool dragging;
	QPoint dragStart, dragOffset;
	QRubberBand *rubberBand;

	QWidget *mainWidget;
	QHBoxLayout *headerLayout;
	QVBoxLayout *widgetLayout;
	QWidget *headerWidget;
	QLabel *titleLabel;
	QToolButton *closeButton;
};

#endif    // _DOCKABLEWIDGET_H
