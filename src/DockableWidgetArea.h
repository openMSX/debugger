#ifndef DOCKABLETWIDGETAREA_H
#define DOCKABLETWIDGETAREA_H

#include "DockableWidgetLayout.h"
#include <QWidget>

class DockableWidget;
class QPaintEvent;

class DockableWidgetArea : public QWidget
{
	Q_OBJECT;
public:
	DockableWidgetArea(QWidget* parent = 0);

private:
	void paintEvent(QPaintEvent* e);

	void removeWidget(DockableWidget* widget);
	void addWidget(DockableWidget* widget, const QRect& rect);
	void addWidget(DockableWidget* widget, DockableWidgetLayout::DockSide side,
	               int distance, int width = -1, int height = -1);
	bool insertLocation(QRect& r, const QSizePolicy& sizePol);
	void getConfig(QStringList& list);

	DockableWidgetLayout* layout;

	friend class DockManager;
};

#endif // DOCKABLETWIDGETAREA_H
