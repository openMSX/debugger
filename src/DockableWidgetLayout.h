// $Id$

#ifndef _DOCKABLEWIDGETLAYOUT_H
#define _DOCKABLEWIDGETLAYOUT_H

#include <QLayout>
#include <QRect>

class QLayoutItem;
class DockableWidget;

class DockableWidgetLayout : public QLayout
{
	Q_OBJECT;
public:
	DockableWidgetLayout(QWidget *parent = 0, int margin = 0, int spacing = -1);
	DockableWidgetLayout(int spacing);
	~DockableWidgetLayout();

	enum DockSide { TOP, LEFT, RIGHT, BOTTOM };
		
	class DockInfo
	{
	public:
		DockableWidget *widget;
		QLayoutItem *item;
		DockSide dockSide;
		int dockDistance;
		bool useHintHeight;
		bool useHintWidth;
		int	left;
		int	top;
		int width;
		int height;
		int distExtra;
	};

	void addItem( QLayoutItem *item );
	void addItem( QLayoutItem *item, int index, DockSide side = RIGHT, int dist = 0, int w = -1, int h = -1 );
	void addWidget( DockableWidget *widget, const QRect& rect );
	void addWidget( DockableWidget *widget, DockSide side, int distance, int width = -1, int height = -1 );
	bool insertLocation( QRect& rect, const QSizePolicy& sizePol );
	
	QLayoutItem *itemAt(int index) const;
	QLayoutItem *takeAt(int index);
	int count() const;

	Qt::Orientations expandingDirections() const;
	bool hasHeightForWidth() const;

	QSize minimumSize() const;
	void setGeometry(const QRect &rect);

	QSize sizeHint() const;

private:
	QList<DockInfo *> dockedWidgets;
	int layoutWidth, layoutHeight;
	int minWidth, minHeight;
	int minBaseWidgetWidth, minBaseWidgetHeight;
	
	void calcMinimumSize();
	void sizeMove( int dx, int dy );
	void doLayout( bool calcMin = false );
	bool insertLocation( QRect& rect, int& index, DockSide& side, const QSizePolicy& sizePol );
};

#endif    // _DOCKABLEWIDGETLAYOUT_H
