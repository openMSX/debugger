#include "DockableWidgetLayout.h"
#include "DockableWidget.h"
#include <QLayoutItem>
#include <QtGlobal>
#include <QSet>


static const int SNAP_DISTANCE = 16;

DockableWidgetLayout::DockableWidgetLayout(QWidget* parent, int margin, int spacing)
	: QLayout(parent)
{
	setMargin(margin);
	setSpacing(spacing);
	minWidth = minHeight = 0;
	maxWidth = maxHeight = 0;
}

DockableWidgetLayout::DockableWidgetLayout(int spacing)
{
	setSpacing(spacing);
	minWidth = minHeight = 0;
	maxWidth = maxHeight = 0;
}

DockableWidgetLayout::~DockableWidgetLayout()
{
	while (!dockedWidgets.empty()) {
		DockInfo* info = dockedWidgets.takeFirst();
		delete info->item;
		delete info->widget;
		delete info;
	}
}

void DockableWidgetLayout::addItem(QLayoutItem* item)
{
	addItem(item, -1);
}

void DockableWidgetLayout::addItem(
		QLayoutItem* item, int index, DockSide side, int dist, int w, int h)
{
	DockInfo* info = new DockInfo();
	info->item = item;
	info->widget = qobject_cast<DockableWidget*>(item->widget());
	info->dockSide = side;
	info->dockDistance = dist;
	info->left = 0;
	info->top = 0;
	info->width = -1;
	info->height = -1;
	info->useHintWidth = true;
	info->useHintHeight = true;

	if (info->widget->sizePolicy().horizontalPolicy() != QSizePolicy::Fixed &&
	    w > 0) {
		info->width = w;
		info->useHintWidth = false;
	}
	if (info->widget->sizePolicy().verticalPolicy() != QSizePolicy::Fixed &&
	    h > 0 ) {
		info->height = h;
		info->useHintHeight = false;
	}

	// first widget is the resize widget, set initial size
	if (dockedWidgets.empty()) {
		if (info->width == -1) {
			info->width = item->sizeHint().width();
		}
		if (info->height == -1) {
			info->height = item->sizeHint().height();
		}
		info->useHintWidth = false;
		info->useHintHeight = false;
	}

	for (int i = 0; i < dockedWidgets.size(); ++i) {
		Q_ASSERT(dockedWidgets.at(i)->widget != item->widget());
	}

	if (index > -1 && index < dockedWidgets.size()) {
		dockedWidgets.insert(index, info);
	} else {
		dockedWidgets.append(info);
	}

	// recalculate size limits
	calcSizeLimits();
}

void DockableWidgetLayout::addWidget(DockableWidget* widget, const QRect& rect)
{
	int index;
	DockSide side;
	QRect r(rect);
	if (insertLocation(r, index, side, widget->sizePolicy())) {
		DockInfo* d = dockedWidgets.first();
		int dist;
		switch (side) {
		case TOP:
		case BOTTOM:
			dist = r.left() - d->left;
			break;
		case LEFT:
		case RIGHT:
			dist = r.top() - d->top;
			break;
		}
		widget->show();
		addItem(new QWidgetItem(widget), index, side, dist,
		        r.width(), r.height());
		update();
	}
}

void DockableWidgetLayout::addWidget(
		DockableWidget* widget, DockSide side, int dist, int w, int h)
{
	// append item at the back
	addItem(new QWidgetItem(widget), -1, side, dist, w, h);
}

void DockableWidgetLayout::changed()
{
	calcSizeLimits();
	update();
}

Qt::Orientations DockableWidgetLayout::expandingDirections() const
{
	return Qt::Horizontal | Qt::Vertical;
}

bool DockableWidgetLayout::hasHeightForWidth() const
{
	return false;
}

int DockableWidgetLayout::count() const
{
	return dockedWidgets.size();
}

QLayoutItem* DockableWidgetLayout::takeAt(int index)
{
	if (index < 0 || index >= dockedWidgets.size()) return 0;

	DockInfo* info = dockedWidgets.takeAt(index);
	QLayoutItem* item = info->item;
	delete info;

	calcSizeLimits();
	return item;
}

QLayoutItem* DockableWidgetLayout::itemAt(int index) const
{
	if (index < 0 || index >= dockedWidgets.size()) return 0;

	return dockedWidgets.at(index)->item;
}

QSize DockableWidgetLayout::minimumSize() const
{
	return QSize(minWidth, minHeight);
}

QSize DockableWidgetLayout::maximumSize() const
{
	return QSize(maxWidth, maxHeight);
}

QSize DockableWidgetLayout::sizeHint() const
{
	return QSize(layoutWidth, layoutHeight);
}

void DockableWidgetLayout::setGeometry(const QRect& rect)
{
	QLayout::setGeometry(rect);

	// Qt sometimes sets the geometry outside the minimumSize/maximumSize range. :/
	int W = std::min(maxWidth,  std::max(minWidth,  rect.width ()));
	int H = std::min(maxHeight, std::max(minHeight, rect.height()));

	// set main widget size
	int dx = W - layoutWidth;
	int dy = H - layoutHeight;

	if (dx != 0 || dy != 0) {
		sizeMove(dx, dy);
		calcSizeLimits();
	}

	// resize the widgets
	for (int i = 0; i < dockedWidgets.size(); ++i) {
		DockInfo* d = dockedWidgets[i];
		if (!d->widget->isHidden()) {
			d->item->setGeometry(d->bounds());
		}
	}
}

void DockableWidgetLayout::calcSizeLimits()
{
	if (dockedWidgets.empty()) return;

	// layout with current sizes
	doLayout();
	DockInfo* d = dockedWidgets.first();

	// store current size
	int curWidth = d->width;
	int curHeight = d->height;
	QVector<int> distStore;
	for (int i = 0; i < dockedWidgets.size(); ++i) {
		distStore.push_back(dockedWidgets.at(i)->dockDistance);
	}

	// first check minimum width (blunt method)
	for (int i = d->widget->minimumWidth(); i <= curWidth; ++i) {
		// trial layout
		sizeMove(i - d->width, 0);
		doLayout(true);
		// restore
		d->width = curWidth;
		for (int j = 1; j < dockedWidgets.size(); ++j) {
			dockedWidgets.at(j)->dockDistance = distStore.at(j);
		}
		// check result
		if (layoutHeight == checkHeight &&
		    layoutWidth - checkWidth == d->width - i) {
			break;
		}
	}
	minWidth = checkWidth;

	// first check maximum width (blunt method)
	for (int i = d->widget->maximumWidth(); i >= curWidth; --i) {
		// trial layout
		sizeMove(i - d->width, 0);
		doLayout(true);
		// restore
		d->width = curWidth;
		for (int j = 1; j < dockedWidgets.size(); ++j) {
			dockedWidgets.at(j)->dockDistance = distStore.at(j);
		}
		// check result
		if (layoutHeight == checkHeight &&
		    layoutWidth - checkWidth == d->width - i) {
			break;
		}
	}
	maxWidth = checkWidth;

	// first check minimum height (blunt method)
	for (int i = d->widget->minimumHeight(); i <= curHeight; ++i) {
		// trial layout
		sizeMove(0, i - d->height);
		doLayout(true);
		// restore
		d->height = curHeight;
		for (int j = 1; j < dockedWidgets.size(); ++j) {
			dockedWidgets.at(j)->dockDistance = distStore.at(j);
		}
		// check result
		if (layoutWidth == checkWidth &&
		    layoutHeight - checkHeight == d->height - i) {
			break;
		}
	}
	minHeight = checkHeight;

	// first check maximum width (blunt method)
	for (int i = d->widget->maximumHeight(); i >= curHeight; --i) {
		// trial layout
		sizeMove(0, i - d->height);
		doLayout(true);
		// restore
		d->height = curHeight;
		for (int j = 1; j < dockedWidgets.size(); ++j) {
			dockedWidgets.at(j)->dockDistance = distStore.at(j);
		}
		// check result
		if (layoutWidth == checkWidth &&
		    layoutHeight - checkHeight == d->height - i) {
			break;
		}
	}
	maxHeight = checkHeight;

	// restore layout
	doLayout();
}

void DockableWidgetLayout::sizeMove(int dx, int dy)
{
	DockInfo* d0 = dockedWidgets.first();
	for (int i = 1; i < dockedWidgets.size(); ++i) {
		DockInfo* d = dockedWidgets.at(i);
		if (d->dockSide == TOP || d->dockSide == BOTTOM) {
			if (d->dockDistance >= d0->width) {
				d->dockDistance += dx;
			}
		}
		if (d->dockSide == LEFT || d->dockSide == RIGHT) {
			if (d->dockDistance >= d0->height) {
				d->dockDistance += dy;
			}
		}
	}
	d0->width += dx;
	d0->height += dy;
}

void DockableWidgetLayout::doLayout(bool check)
{
	if (dockedWidgets.empty()) return;

	DockInfo* d = dockedWidgets.first();
	d->left = 0;
	d->top = 0;
	int W = d->width;
	int H = d->height;

	int dx = 0, dy = 0;
	for (int i = 1; i < dockedWidgets.size(); ++i) {
		d = dockedWidgets[i];
		// only process visible widgets
		if (d->widget->isHidden()) {
			d->left = -10000;
			d->top = -10000;
			continue;
		}
		// determine size
		if (d->useHintWidth) {
			d->width = d->item->sizeHint().width();
		}
		if (d->useHintHeight) {
			d->height = d->item->sizeHint().height();
		}
		// determine location
		switch (d->dockSide) {
		case TOP:
			d->left = d->dockDistance;
			if (d->dockDistance >= W || d->dockDistance+d->width <= 0) {
				d->top = H - d->height;
			} else {
				d->top = -d->height;
			}
			// adjust position until it doesn't overlap other widgets
			for (int j = 1; j < i; ++j) {
				DockInfo* d2 = dockedWidgets[j];
				QRect r(d->left,  d->top    - QWIDGETSIZE_MAX,
					d->width, d->height + QWIDGETSIZE_MAX);
				if (r.intersects(d2->bounds())) {
					d->top = d2->top - d->height;
				}
			}
			break;
		case LEFT:
			d->top = d->dockDistance;
			if (d->dockDistance >= H || d->dockDistance+d->height <= 0) {
				d->left = W - d->width;
			} else {
				d->left = -d->width;
			}
			// adjust position until it doesn't overlap other widgets
			for (int j = 1; j < i; ++j) {
				DockInfo* d2 = dockedWidgets[j];
				QRect r(d->left  - QWIDGETSIZE_MAX, d->top,
				        d->width + QWIDGETSIZE_MAX, d->height);
				if (r.intersects(d2->bounds())) {
					d->left = d2->left - d->width;
				}
			}
			break;
		case RIGHT:
			d->top = d->dockDistance;
			if (d->dockDistance >= H || d->dockDistance+d->height <= 0) {
				d->left = 0;
			} else {
				d->left = W;
			}
			// adjust position until it doesn't overlap other widgets
			for (int j = 1; j < i; ++j) {
				DockInfo* d2 = dockedWidgets[j];
				QRect r(d->left, d->top,
				        d->width + QWIDGETSIZE_MAX, d->height);
				if (r.intersects(d2->bounds())) {
					d->left = d2->left + d2->width;
				}
			}
			break;
		case BOTTOM:
			d->left = d->dockDistance;
			if (d->dockDistance >= W || d->dockDistance+d->width <= 0) {
				d->top = 0;
			} else {
				d->top = H;
			}
			// adjust position until it doesn't overlap other widgets
			for (int j = 1; j < i; ++j) {
				DockInfo* d2 = dockedWidgets[j];
				QRect r(d->left, d->top,
				        d->width, d->height + QWIDGETSIZE_MAX);
				if (r.intersects(d2->bounds())) {
					d->top = d2->top + d2->height;
				}
			}
			break;
		}
		// check negative coordinates
		if (d->left < dx) dx = d->left;
		if (d->top  < dy) dy = d->top;
	}

	// translate widgets and calculate size
	int& w = check ? checkWidth  : layoutWidth;
	int& h = check ? checkHeight : layoutHeight;
	w = h = 0;
	for (int i = 0; i < dockedWidgets.size(); ++i) {
		DockInfo* d = dockedWidgets[i];
		if (!d->widget->isHidden()) {
			d->left -= dx;
			d->top  -= dy;
			w = std::max(w, d->right());
			h = std::max(h, d->bottom());
		}
	}
}

bool DockableWidgetLayout::overlaysWithFirstNWidgets(const QRect& r, int n) const
{
	for (int i = 0; i < n; ++i) {
		if (r.intersects(dockedWidgets[i]->bounds())) {
			return true;
		}
	}
	return false;
}

static bool isClose(int a, int b)
{
	return abs(a - b) < SNAP_DISTANCE;
}

bool DockableWidgetLayout::insertLocation(
	QRect& rect, int& index, DockSide& side, const QSizePolicy& sizePol)
{
	// best insertion data
	// Distance is a number that represents the how far
	// the insertion rectangle is from the final location.
	unsigned bestDistance = 0xFFFFFFFF;
	int bestIndex = 0;
	DockSide bestSide;
	QRect bestRect;

	// loop over all widgets and find appropriate matching sides
	for (int i = 0; i < dockedWidgets.size(); ++i) {
		DockInfo* d = dockedWidgets[i];
		/*****************************************************
		 * Check for placement against the top of the widget *
		 *****************************************************/
		if (i == 0 || d->dockSide != BOTTOM) {
			if (!(rect.left()  > d->right() - SNAP_DISTANCE ||
			      rect.right() < d->left    + SNAP_DISTANCE) &&
			    isClose(rect.bottom(), d->top)) {
				// rectangle is close to the edge
				unsigned dist = 8 * abs(rect.bottom() - d->top);
				// now find all points on this side
				// (use set as a sorted unique list)
				QSet<int> sidePoints;
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->top == d2->top) {
						sidePoints.insert(d2->left);
						sidePoints.insert(d2->right());
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->bottom() == d2->top) {
								sidePoints.insert(d3->left);
								sidePoints.insert(d3->right());
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				auto it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					unsigned newDist1 = dist + abs(*it - rect.left());
					if (newDist1 < bestDistance && isClose(*it, rect.left())) {
						QRect r(QPoint(*it, d->top - rect.height()), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist1;
							bestIndex = i + 1;
							bestSide = TOP;
							bestRect = r;
						}
					}
					++it;
					// check before point
					unsigned newDist2 = dist + abs(*it - rect.right());
					if (newDist2 < bestDistance && isClose(*it, rect.right())) {
						QRect r(QPoint(*it - rect.width(), d->top - rect.height()),
						        rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist2;
							bestIndex = i + 1;
							bestSide = TOP;
							bestRect = r;
						}
					}
				}
				// check for resized placement options
				if (sizePol.horizontalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.left() + rect.width() / 2;
					for (auto ita = sidePoints.begin() + 1; ita != sidePoints.end(); ++ita) {
						for (auto itb = sidePoints.begin(); ita != itb; ++itb) {
							int sp_mid = (*ita + *itb) / 2;
							int sp_diff = *ita - *itb;
							if (isClose(sp_mid, mid)) {
								QRect r(*itb, d->top - rect.height(),
								        sp_diff, rect.height());
								if (!overlaysWithFirstNWidgets(r, i)) {
									bestDistance = dist + abs(sp_mid - mid);
									bestIndex = i + 1;
									bestSide = TOP;
									bestRect = r;
								}
							}
						}
					}
				}
			}
		}

		/********************************************************
		 * Check for placement against the bottom of the widget *
		 ********************************************************/
		if (i == 0 || d->dockSide != TOP) {
			if (!(rect.left()  > d->right() - SNAP_DISTANCE ||
			      rect.right() < d->left    + SNAP_DISTANCE) &&
			    isClose(rect.top(), d->bottom())) {
				// rectangle is close to the edge
				unsigned dist = 8 * abs(rect.top() - d->bottom());
				// now find all points on this side
				// (use set as a sorted unique list)
				QSet<int> sidePoints;
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->bottom() == d2->bottom()) {
						sidePoints.insert(d2->left);
						sidePoints.insert(d2->right());
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->top == d2->bottom()) {
								sidePoints.insert(d3->left);
								sidePoints.insert(d3->right());
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				auto it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					unsigned newDist1 = dist + abs(*it - rect.left());
					if (newDist1 < bestDistance && isClose(*it, rect.left())) {
						QRect r(QPoint(*it, d->bottom()), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist1;
							bestIndex = i + 1;
							bestSide = BOTTOM;
							bestRect = r;
						}
					}
					++it;
					// check before point
					unsigned newDist2 = dist + abs(*it - rect.right());
					if (newDist2 < bestDistance && isClose(*it, rect.right())) {
						QRect r(QPoint(*it - rect.width(), d->bottom()), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist2;
							bestIndex = i + 1;
							bestSide = BOTTOM;
							bestRect = r;
						}
					}
				}
				// check for resized placement options
				if (sizePol.horizontalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.left() + rect.width() / 2;
					for (auto ita = sidePoints.begin() + 1; ita != sidePoints.end(); ++ita) {
						for (auto itb = sidePoints.begin(); ita != itb; ++itb) {
							int sp_mid = (*ita + *itb) / 2;
							int sp_diff = *ita - *itb;
							if (isClose(sp_mid, mid)) {
								QRect r(*itb, d->bottom(),
								        sp_diff, rect.height());
								if (!overlaysWithFirstNWidgets(r, i)) {
									bestDistance = dist + abs(sp_mid - mid);
									bestIndex = i + 1;
									bestSide = BOTTOM;
									bestRect = r;
								}
							}
						}
					}
				}
			}
		}

		/******************************************************
		 * Check for placement against the left of the widget *
		 ******************************************************/
		if (i == 0 || d->dockSide != RIGHT) {
			if (!(rect.top()    > d->bottom() - SNAP_DISTANCE ||
			      rect.bottom() < d->top      + SNAP_DISTANCE) &&
			    isClose(rect.right(), d->left)) {
				// rectangle is close to the edge
				unsigned dist = 8 * abs(rect.right() - d->left);
				// now find all points on this side
				// (use set as a sorted unique list)
				QSet<int> sidePoints;
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->left == d2->left) {
						sidePoints.insert(d2->top);
						sidePoints.insert(d2->bottom());
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->right() == d2->left) {
								sidePoints.insert(d3->top);
								sidePoints.insert(d3->bottom());
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				auto it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					unsigned newDist1 = dist + abs(*it - rect.top());
					if (newDist1 < bestDistance && isClose(*it, rect.top())) {
						QRect r(QPoint(d->left - rect.width(), *it), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist1;
							bestIndex = i + 1;
							bestSide = LEFT;
							bestRect = r;
						}
					}
					++it;
					// check before point
					unsigned newDist2 = dist + abs(*it - rect.bottom());
					if (newDist2 < bestDistance && isClose(*it, rect.bottom())) {
						QRect r(QPoint(d->left - rect.width(), *it - rect.height()),
						        rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist2;
							bestIndex = i + 1;
							bestSide = LEFT;
							bestRect = r;
						}
					}
				}
				// check for resized placement options
				if (sizePol.verticalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.top() + rect.height() / 2;
					for (auto ita = sidePoints.begin() + 1; ita != sidePoints.end(); ++ita) {
						for (auto itb = sidePoints.begin(); ita != itb; ++itb) {
							int sp_mid = (*ita + *itb) / 2;
							int sp_diff = *ita - *itb;
							if (isClose(sp_mid, mid)) {
								QRect r(d->left - rect.width(), *itb,
								        rect.width(), sp_diff);
								if (!overlaysWithFirstNWidgets(r, i)) {
									bestDistance = dist + abs(sp_mid - mid);
									bestIndex = i + 1;
									bestSide = LEFT;
									bestRect = r;
								}
							}
						}
					}
				}
			}
		}
		/*******************************************************
		 * Check for placement against the right of the widget *
		 *******************************************************/
		if (i == 0 || d->dockSide != LEFT) {
			if (!(rect.top()    > d->bottom() - SNAP_DISTANCE ||
			      rect.bottom() < d->top      + SNAP_DISTANCE) &&
			    isClose(rect.left(), d->right())) {
				// rectangle is close to the edge
				unsigned dist = 8 * abs(rect.left() - d->right());
				// now find all points on this side
				// (use set as a sorted unique list)
				QSet<int> sidePoints;
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->right() == d2->right()) {
						sidePoints.insert(d2->top);
						sidePoints.insert(d2->bottom());
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->left == d2->right()) {
								sidePoints.insert(d3->top);
								sidePoints.insert(d3->bottom());
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				auto it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					unsigned newDist1 = dist + abs(*it - rect.top());
					if (newDist1 < bestDistance && isClose(*it, rect.top())) {
						QRect r(QPoint(d->left + d->width, *it), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist1;
							bestIndex = i + 1;
							bestSide = RIGHT;
							bestRect = r;
						}
					}
					++it;
					// check before point
					unsigned newDist2 = dist + abs(*it - rect.bottom());
					if (newDist2 < bestDistance && isClose(*it, rect.bottom())) {
						QRect r(QPoint(d->right(), *it - rect.height()), rect.size());
						if (!overlaysWithFirstNWidgets(r, i)) {
							bestDistance = newDist2;
							bestIndex = i + 1;
							bestSide = RIGHT;
							bestRect = r;
						}
					}
				}
				// check for resized placement options
				if (sizePol.verticalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.top() + rect.height() / 2;
					for (auto ita = sidePoints.begin() + 1; ita != sidePoints.end(); ++ita) {
						for (auto itb = sidePoints.begin(); ita != itb; ++itb) {
							int sp_mid = (*ita + *itb) / 2;
							int sp_diff = *ita - *itb;
							if (isClose(sp_mid, mid)) {
								QRect r(d->right(), *itb, rect.width(), sp_diff);
								if (!overlaysWithFirstNWidgets(r, i)) {
									bestDistance = dist + abs(sp_mid - mid);
									bestIndex = i + 1;
									bestSide = RIGHT;
									bestRect = r;
								}
							}
						}
					}
				}
			}
		}
	}

	if (bestIndex) {
		rect = bestRect;
		index = bestIndex;
		side = bestSide;
		return true;
	} else {
		return false;
	}
}

bool DockableWidgetLayout::insertLocation(QRect& rect, const QSizePolicy& sizePol)
{
	int index;
	DockSide side;
	return insertLocation(rect, index, side, sizePol);
}

void DockableWidgetLayout::getConfig(QStringList& list)
{
	for (int i = 0; i < dockedWidgets.size(); ++i) {
		DockInfo* d = dockedWidgets.at(i);

		// string format D [Hidden/Visible] [Side] [Distance] [Width] [Height]
		QString s("%1 D %2 %3 %4 %5 %6");
		s = s.arg(d->widget->id());

		if (d->widget->isHidden()) {
			s = s.arg("H");
		} else {
			s = s.arg("V");
		}

		switch (d->dockSide) {
		case TOP:
			s = s.arg("T");
			break;
		case LEFT:
			s = s.arg("L");
			break;
		case RIGHT:
			s = s.arg("R");
			break;
		case BOTTOM:
			s = s.arg("B");
			break;
		}

		s = s.arg(d->dockDistance);

		if (d->useHintWidth) {
			s = s.arg(-1);
		} else {
			s = s.arg(d->width);
		}

		if (d->useHintHeight) {
			s = s.arg(-1);
		} else {
			s = s.arg(d->height);
		}
		list.append(s);
	}
}
