#ifndef DOCKABLEWIDGETLAYOUT_H
#define DOCKABLEWIDGETLAYOUT_H

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

	DockableWidgetLayout(QWidget* parent = nullptr, int margin = 0, int spacing = -1);
	DockableWidgetLayout(int spacing);
	~DockableWidgetLayout() override;

	void addItem(QLayoutItem* item) override;
	void addItem(QLayoutItem* item, int index, DockSide side = RIGHT,
	             int dist = 0, int w = -1, int h = -1);
	void addWidget(DockableWidget* widget, const QRect& rect);
	void addWidget(DockableWidget* widget, DockSide side, int distance,
	               int width = -1, int height = -1);
	bool insertLocation(QRect& rect, const QSizePolicy& sizePol);

	QLayoutItem* itemAt(int index) const override;
	QLayoutItem* takeAt(int index) override;
	int count() const override;

	Qt::Orientations expandingDirections() const override;
	bool hasHeightForWidth() const override;

	QSize minimumSize() const override;
	QSize maximumSize() const override;
	void setGeometry(const QRect &rect) override;

	QSize sizeHint() const override;
	void changed();

	void getConfig(QStringList& list);

private:
	class DockInfo
	{
	public:
		QRect bounds() const { return {left, top, width, height}; }
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

#endif // DOCKABLEWIDGETLAYOUT_H
