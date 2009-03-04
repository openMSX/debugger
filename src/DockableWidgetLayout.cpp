// $Id$

#include "DockableWidgetLayout.h"
#include "DockableWidget.h"
#include <QLayoutItem>
#include <QtGlobal>
#include <QMap>


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

bool DockableWidgetLayout::insertLocation(
	QRect& rect, int& index, DockSide& side, const QSizePolicy& sizePol)
{
	// best insertion data
	// Distance is a number that represents the how far
	// the insertion rectangle is from the final location.
	unsigned int bestDistance = 0xFFFFFFFF;
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
			    abs(rect.bottom() - d->top) < SNAP_DISTANCE) {
				// rectangle is close to the edge
				unsigned int dist = 8 * abs(rect.bottom() - d->top);
				// now find all points on this side
				// (abuse map to get sorted unique list)
				QMap<int, int> sidePoints;
				// loop over all widgets
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->top == d2->top) {
						// add the two edges
						sidePoints.insert(d2->left, 0);
						sidePoints.insert(d2->right(), 0);
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->bottom() == d2->top) {
								// add the two edges
								sidePoints.insert(d3->left, 0);
								sidePoints.insert(d3->right(), 0);
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				QMap<int, int>::iterator it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					if (dist + abs(it.key() - rect.left()) < bestDistance &&
					    abs(it.key() - rect.left()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(it.key(), d->top - rect.height()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.left());
							bestIndex = i + 1;
							bestSide = TOP;
							bestRect = rect;
							bestRect.moveBottomLeft(QPoint(it.key(), d->top - 1));
						}
					}
					++it;
					// check before point
					if (dist + abs(it.key() - rect.right()) < bestDistance &&
					    abs(it.key() - rect.right()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(it.key() - rect.width(), d->top - rect.height()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.right());
							bestIndex = i + 1;
							bestSide = TOP;
							bestRect = rect;
							bestRect.moveBottomRight(QPoint(it.key() - 1, d->top - 1));
						}
					}
				}
				// check for resized placement options
				if (sizePol.horizontalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.left() + rect.width() / 2;
					QMap<int, int>::iterator ita = sidePoints.begin() + 1;
					while (ita != sidePoints.end()) {
						QMap<int, int>::iterator itb = sidePoints.begin();
						while (ita != itb) {
							// check if rect middle is close to the middle between two points
							if (abs((ita.key() + itb.key()) / 2 - mid) < SNAP_DISTANCE) {
								// verify if this area is free
								int k;
								for (k = 0; k < i; ++k) {
									DockInfo* d3 = dockedWidgets[k];
									if (QRect(itb.key(), d->top - rect.height(), ita.key() - itb.key(), rect.height())
									       .intersects(d3->bounds())) {
										break;
									}
								}
								if (k == i) {
									bestDistance = dist + abs((ita.key() + itb.key()) / 2 - mid);
									bestIndex = i + 1;
									bestSide = TOP;
									bestRect = QRect(itb.key(), d->top - rect.height(),
									                 ita.key() - itb.key(), rect.height());
								}
							}
							++itb;
						}
						++ita;
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
			    abs(rect.top() - d->bottom()) < SNAP_DISTANCE) {
				// rectangle is close to the edge
				unsigned int dist = 8 * abs(rect.top() - d->bottom());
				// now find all points on this side
				// (abuse map to get sorted unique list)
				QMap<int, int> sidePoints;
				// loop over all widgets
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->bottom() == d2->bottom()) {
						// add the two edges
						sidePoints.insert(d2->left, 0);
						sidePoints.insert(d2->right(), 0);
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->top == d2->bottom()) {
								// add the two edges
								sidePoints.insert(d3->left, 0);
								sidePoints.insert(d3->right(), 0);
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				QMap<int, int>::iterator it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					if (dist + abs(it.key() - rect.left()) < bestDistance &&
					    abs(it.key() - rect.left()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(it.key(), d->bottom()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.left());
							bestIndex = i + 1;
							bestSide = BOTTOM;
							bestRect = rect;
							bestRect.moveTopLeft(QPoint(it.key(), d->bottom()));
						}
					}
					++it;
					// check before point
					if (dist + abs(it.key() - rect.right()) < bestDistance &&
					    abs(it.key() - rect.right()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(it.key() - rect.width(), d->bottom()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.right());
							bestIndex = i + 1;
							bestSide = BOTTOM;
							bestRect = rect;
							bestRect.moveTopRight(QPoint(it.key() - 1, d->bottom()));
						}
					}
				}
				// check for resized placement options
				if (sizePol.horizontalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.left() + rect.width() / 2;
					QMap<int, int>::iterator ita = sidePoints.begin() + 1;
					while (ita != sidePoints.end()) {
						QMap<int, int>::iterator itb = sidePoints.begin();
						while (ita != itb) {
							// check if rect middle is close to the middle between two points
							if (abs((ita.key() + itb.key()) / 2 - mid) < SNAP_DISTANCE) {
								// verify if this area is free
								int k;
								for (k = 0; k < i; ++k) {
									DockInfo* d3 = dockedWidgets[k];
									if (QRect(itb.key(), d->bottom(), ita.key() - itb.key(), rect.height())
									       .intersects(d3->bounds())) {
										break;
									}
								}
								if (k == i) {
									bestDistance = dist + abs((ita.key() + itb.key()) / 2 - mid);
									bestIndex = i + 1;
									bestSide = BOTTOM;
									bestRect = QRect(itb.key(), d->bottom(), ita.key() - itb.key(), rect.height());
								}
							}
							++itb;
						}
						++ita;
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
			    abs(rect.right() - d->left) < SNAP_DISTANCE) {
				// rectangle is close to the edge
				unsigned int dist = 8 * abs(rect.right() - d->left);
				// now find all points on this side
				// (abuse map to get sorted unique list)
				QMap<int, int> sidePoints;
				// loop over all widgets
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->left == d2->left) {
						// add the two edges
						sidePoints.insert(d2->top, 0);
						sidePoints.insert(d2->bottom(), 0);
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->right() == d2->left) {
								// add the two edges
								sidePoints.insert(d3->top, 0);
								sidePoints.insert(d3->bottom(), 0);
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				QMap<int, int>::iterator it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					if (dist + abs(it.key() - rect.top()) < bestDistance &&
					    abs(it.key() - rect.top()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(d->left - rect.width(), it.key()), rect.size())
							       .intersects(d3->bounds()))
								break;
							
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.top());
							bestIndex = i + 1;
							bestSide = LEFT;
							bestRect = rect;
							bestRect.moveTopRight(QPoint(d->left - 1, it.key()));
						}
					}
					++it;
					// check before point
					if (dist + abs(it.key() - rect.bottom()) < bestDistance &&
					    abs(it.key() - rect.bottom()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(d->left - rect.width(), it.key() - rect.height()), rect.size())
							       .intersects(d3->bounds()))
								break;
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.bottom());
							bestIndex = i + 1;
							bestSide = LEFT;
							bestRect = rect;
							bestRect.moveBottomRight(QPoint(d->left - 1, it.key() - 1));
						}
					}
				}
				// check for resized placement options
				if (sizePol.verticalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.top() + rect.height() / 2;
					QMap<int, int>::iterator ita = sidePoints.begin() + 1;
					while (ita != sidePoints.end()) {
						QMap<int, int>::iterator itb = sidePoints.begin();
						while (ita != itb) {
							// check if rect middle is close to the middle between two points
							if (abs((ita.key() + itb.key()) / 2 - mid) < SNAP_DISTANCE) {
								// verify if this area is free
								int k;
								for (k = 0; k < i; ++k) {
									DockInfo* d3 = dockedWidgets[k];
									if (QRect(d->left - rect.width(), itb.key(), rect.width(), ita.key() - itb.key())
									       .intersects(d3->bounds())) {
										break;
									}
								}
								if (k == i) {
									bestDistance = dist + abs((ita.key() + itb.key()) / 2 - mid);
									bestIndex = i + 1;
									bestSide = LEFT;
									bestRect = QRect(d->left - rect.width(), itb.key(), rect.width(), ita.key() - itb.key());
								}
							}
							++itb;
						}
						++ita;
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
			    abs(rect.left() - d->right()) < SNAP_DISTANCE) {
				// rectangle is close to the edge
				unsigned int dist = 8 * abs(rect.left() - d->right());
				// now find all points on this side
				// (abuse map to get sorted unique list)
				QMap<int, int> sidePoints;
				// loop over all widgets
				for (int j = 0; j <= i; ++j) {
					DockInfo* d2 = dockedWidgets[j];
					if (d->right() == d2->right()) {
						// add the two edges
						sidePoints.insert(d2->top, 0);
						sidePoints.insert(d2->bottom(), 0);
						// check if any other widget rest against this side
						for (int k = i + 1; k < dockedWidgets.size(); ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (d3->left == d2->right()) {
								// add the two edges
								sidePoints.insert(d3->top, 0);
								sidePoints.insert(d3->bottom(), 0);
							}
						}
					}
				}
				// widget placement can occur at all points, find the closest
				QMap<int, int>::iterator it = sidePoints.begin();
				for (int j = 0; j < sidePoints.size() - 1; ++j) {
					// check after point
					if (dist + abs(it.key() - rect.top()) < bestDistance &&
					    abs(it.key() - rect.top()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(d->left + d->width, it.key()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.top());
							bestIndex = i + 1;
							bestSide = RIGHT;
							bestRect = rect;
							bestRect.moveTopLeft(QPoint(d->right(), it.key()));
						}
					}
					++it;
					// check before point
					if (dist + abs(it.key() - rect.bottom()) < bestDistance &&
					    abs(it.key() - rect.bottom()) < SNAP_DISTANCE) {
						// verify if it does not overlap a placed widget
						int k;
						for (k = 0; k < i; ++k) {
							DockInfo* d3 = dockedWidgets[k];
							if (QRect(QPoint(d->right(), it.key()-rect.height()), rect.size())
							       .intersects(d3->bounds())) {
								break;
							}
						}
						if (k == i) {
							bestDistance = dist + abs(it.key() - rect.bottom());
							bestIndex = i + 1;
							bestSide = RIGHT;
							bestRect = rect;
							bestRect.moveBottomLeft(QPoint(d->right(), it.key() - 1));
						}
					}
				}
				// check for resized placement options
				if (sizePol.verticalPolicy() != QSizePolicy::Fixed) {
					int mid = rect.top() + rect.height() / 2;
					QMap<int, int>::iterator ita = sidePoints.begin() + 1;
					while (ita != sidePoints.end()) {
						QMap<int, int>::iterator itb = sidePoints.begin();
						while (ita != itb) {
							// check if rect middle is close to the middle between two points
							if (abs((ita.key() + itb.key()) / 2 - mid) < SNAP_DISTANCE) {
								// verify if this area is free
								int k;
								for (k = 0; k < i; ++k) {
									DockInfo* d3 = dockedWidgets[k];
									if (QRect(d->right(), itb.key(), rect.width(), ita.key() - itb.key())
									       .intersects(d3->bounds())) {
										break;
									}
								}
								if (k == i) {
									bestDistance = dist + abs((ita.key() + itb.key()) / 2 - mid);
									bestIndex = i + 1;
									bestSide = RIGHT;
									bestRect = QRect(d->right(), itb.key(), rect.width(), ita.key() - itb.key());
								}
							}
							++itb;
						}
						++ita;
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
