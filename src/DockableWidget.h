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
	DockableWidget(DockManager& manager, QWidget* parent = 0);
	~DockableWidget();

	QWidget* widget() const;
	void setWidget(QWidget* widget);
	const QString& id() const;
	void setId(const QString& str);

	QString title() const;
	void setTitle(const QString& title);

	bool isFloating() const;
	void setFloating(bool enable, bool showNow = true);
	bool isMovable() const;
	void setMovable(bool enable);
	bool isClosable() const;
	void setClosable(bool enable);
	bool isDestroyable() const;
	void setDestroyable(bool enable);

private:
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void closeEvent(QCloseEvent* event);

	DockManager& dockManager;
	QString widgetId;

	bool floating;
	bool closable, movable, destroyable;
	bool dragging;
	QPoint dragStart, dragOffset;
	QRubberBand* rubberBand;

	QWidget* mainWidget;
	QHBoxLayout* headerLayout;
	QVBoxLayout* widgetLayout;
	QWidget* headerWidget;
	QLabel* titleLabel;
	QToolButton* closeButton;
	
signals:
	void visibilityChanged(DockableWidget* w);
};

#endif // _DOCKABLEWIDGET_H
