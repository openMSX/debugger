// $Id$

#ifndef _DOCKABLEWIDGETLAYOUT_H
#define _DOCKABLEWIDGETLAYOUT_H

#include <QLayout>
#include <QRect>

class QLayoutItem;
class DockableWidget;
class QStringList;

class DockableWidgetLayout : public QLayout
{
	Q_OBJECT;
public:
	enum DockSide { TOP, LEFT, RIGHT, BOTTOM };

	DockableWidgetLayout(QWidget* parent = 0, int margin = 0, int spacing = -1);
	DockableWidgetLayout(int spacing);
	~DockableWidgetLayout();

	void addItem(QLayoutItem* item);
	void addItem(QLayoutItem* item, int index, DockSide side = RIGHT,
	             int dist = 0, int w = -1, int h = -1);
	void addWidget(DockableWidget* widget, const QRect& rect);
	void addWidget(DockableWidget* widget, DockSide side, int distance,
	               int width = -1, int height = -1);
	bool insertLocation(QRect& rect, const QSizePolicy& sizePol);
	
	QLayoutItem* itemAt(int index) const;
	QLayoutItem* takeAt(int index);
	int count() const;

	Qt::Orientations expandingDirections() const;
	bool hasHeightForWidth() const;

	QSize minimumSize() const;
	QSize maximumSize() const;
	void setGeometry(const QRect &rect);

	QSize sizeHint() const;
	void changed();

	void getConfig(QStringList& list);

private:
	class DockInfo
	{
	public:
		QRect bounds() const { return QRect(left, top, width, height); }
		int right()  const { return left + width;  }
		int bottom() const { return top  + height; }

		DockableWidget* widget;
		QLayoutItem* item;
		DockSide dockSide;
		int dockDistance;
		int left;
		int top;
		int width;
		int height;
		bool useHintHeight;
		bool useHintWidth;
	};

	QList<DockInfo*> dockedWidgets;
	int layoutWidth, layoutHeight;
	int minWidth, minHeight;
	int maxWidth, maxHeight;
	int checkWidth, checkHeight;

	void calcSizeLimits();
	void sizeMove(int dx, int dy);
	void doLayout(bool check = false);
	bool insertLocation(QRect& rect, int& index, DockSide& side,
	                    const QSizePolicy& sizePol);
	bool overlaysWithFirstNWidgets(const QRect& r, int n) const;
};

#endif // _DOCKABLEWIDGETLAYOUT_H
